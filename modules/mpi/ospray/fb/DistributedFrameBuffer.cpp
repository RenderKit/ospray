// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedFrameBuffer.h"
#include <snappy.h>
#include <thread>
#include "DistributedFrameBuffer_TileMessages.h"
#include "TileOperation.h"
#include "fb/DistributedFrameBuffer_ispc.h"
#include "common/Profiling.h"

#include "pico_bench.h"
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/tasking/schedule.h"

#include "api/Device.h"
#include "common/Collectives.h"
#include "common/MPICommon.h"

using namespace std::chrono;

namespace ospray {

// Helper types /////////////////////////////////////////////////////////////

using DFB = DistributedFrameBuffer;

// DistributedTileError definitions /////////////////////////////////////////

DistributedTileError::DistributedTileError(
    const vec2i &numTiles, mpicommon::Group group)
    : TileError(numTiles), group(group)
{}

void DistributedTileError::sync()
{
  if (tiles <= 0)
    return;

  mpicommon::bcast(tileErrorBuffer, tiles, MPI_FLOAT, 0, group.comm).wait();
}

// DistributedFrameBuffer definitions ///////////////////////////////////////

DFB::DistributedFrameBuffer(const vec2i &numPixels,
    ObjectHandle myId,
    ColorBufferFormat colorBufferFormat,
    const uint32 channels)
    // TODO WILL: The DFB should use a separate identifier than myID
    // since large scenes can push this ID beyond the max value we can use
    // for MPI Tag (i.e., Moana). The IDs for the message handler should not
    // be set from the object handle but pulled from some other ID pool
    // specific to those objects using the messaging layer
    : MessageHandler(myId),
      FrameBuffer(numPixels, colorBufferFormat, channels),
      mpiGroup(mpicommon::worker.dup()),
      tileErrorRegion(hasVarianceBuffer ? getNumTiles() : vec2i(0), mpiGroup),
      localFBonMaster(nullptr),
      frameIsActive(false),
      frameIsDone(false)
{
  this->ispcEquivalent = ispc::DFB_create(this);
  ispc::DFB_set(getIE(), numPixels.x, numPixels.y, colorBufferFormat);

  // TODO: accumID is eventually only needed on master once static
  // loadbalancing is removed
  tileAccumID.resize(getTotalTiles(), 0);

  if (mpicommon::IamTheMaster() && colorBufferFormat != OSP_FB_NONE) {
    localFBonMaster = rkcommon::make_unique<LocalFrameBuffer>(numPixels,
        colorBufferFormat,
        channels & ~(OSP_FB_ACCUM | OSP_FB_VARIANCE));
  }
}

void DFB::commit()
{
  FrameBuffer::commit();
  tileOperation = nullptr;
  lastRenderer = nullptr;
  allTiles.clear();
  myTiles.clear();

  imageOps.clear();
  if (imageOpData) {
    FrameBufferView fbv(localFBonMaster ? localFBonMaster.get()
                                        : static_cast<FrameBuffer *>(this),
        colorBufferFormat,
        localFBonMaster ? localFBonMaster->colorBuffer : nullptr,
        localFBonMaster ? localFBonMaster->depthBuffer : nullptr,
        localFBonMaster ? localFBonMaster->normalBuffer : nullptr,
        localFBonMaster ? localFBonMaster->albedoBuffer : nullptr);

    std::for_each(imageOpData->begin(), imageOpData->end(), [&](ImageOp *i) {
      if (!dynamic_cast<FrameOp *>(i) || localFBonMaster)
        imageOps.push_back(i->attach(fbv));
    });

    findFirstFrameOperation();
  }
}

void DFB::startNewFrame(const float errorThreshold)
{
  {
    std::lock_guard<std::mutex> lock(finalTileBufferMutex);
    nextTileWrite = 0;
    tileBufferOffsets.clear();
  }
  if (colorBufferFormat != OSP_FB_NONE) {
    // Allocate a conservative upper bound of space which we'd need to
    // store the compressed tiles
    const size_t uncompressedSize = masterMsgSize(
        colorBufferFormat, hasDepthBuffer, hasNormalBuffer, hasAlbedoBuffer);
    const size_t compressedSize = snappy::MaxCompressedLength(uncompressedSize);
    tileGatherBuffer.resize(myTiles.size() * compressedSize, 0);
  }

  {
    std::lock_guard<std::mutex> fbLock(mutex);
    std::lock_guard<std::mutex> numTilesLock(numTilesMutex);

    if (frameIsActive) {
      throw std::runtime_error(
          "Attempt to start frame on already started frame!");
    }

    std::for_each(imageOps.begin(),
        imageOps.end(),
        [](std::unique_ptr<LiveImageOp> &p) { p->beginFrame(); });

    lastProgressReport = std::chrono::steady_clock::now();
    renderingProgressTiles = 0;

    tileErrorRegion.sync();

    if (colorBufferFormat == OSP_FB_NONE) {
      std::lock_guard<std::mutex> errsLock(tileErrorsMutex);
      tileIDs.clear();
      tileErrors.clear();
      tileIDs.reserve(myTiles.size());
      tileErrors.reserve(myTiles.size());
    }

    // May be worth parallelizing if we have tile ops where
    // new frame becomes expensive
    for (auto &tile : myTiles)
      tile->newFrame();

    if (mpicommon::IamTheMaster()) {
      numTilesExpected.resize(mpicommon::workerSize(), 0);
      std::fill(numTilesExpected.begin(), numTilesExpected.end(), 0);
    }

    globalTilesCompletedThisFrame = 0;
    numTilesCompletedThisFrame = 0;
    for (int t = 0; t < getTotalTiles(); t++) {
      const uint32_t nx = static_cast<uint32_t>(getNumTiles().x);
      const uint32_t ty = t / nx;
      const uint32_t tx = t - ty * nx;
      const vec2i tileID(tx, ty);
      if (hasAccumBuffer && tileError(tileID) <= errorThreshold) {
        if (allTiles[t]->mine()) {
          numTilesCompletedThisFrame++;
        }
      } else if (mpicommon::IamTheMaster()) {
        ++numTilesExpected[allTiles[t]->ownerID];
      }
    }

    frameIsDone = false;

    frameIsActive = true;
  }
  mpicommon::barrier(mpiGroup.comm).wait();

  if (isFrameComplete(0))
    closeCurrentFrame();
}

bool DFB::isFrameComplete(const size_t numTiles)
{
  std::lock_guard<std::mutex> numTilesLock(numTilesMutex);
  numTilesCompletedThisFrame += numTiles;

  renderingProgressTiles += numTiles;

  auto now = std::chrono::steady_clock::now();
  auto timeSinceUpdate = duration_cast<milliseconds>(now - lastProgressReport);
  if (timeSinceUpdate.count() >= 1000) {
    auto msg = std::make_shared<mpicommon::Message>(sizeof(ProgressMessage));
    ProgressMessage *msgData = reinterpret_cast<ProgressMessage *>(msg->data);
    msgData->command = PROGRESS_MESSAGE;
    msgData->numCompleted = renderingProgressTiles;
    msgData->frameID = frameID;
    mpi::messaging::sendTo(mpicommon::masterRank(), myId, msg);

    renderingProgressTiles = 0;
    lastProgressReport = now;
  }

  return numTilesCompletedThisFrame == myTiles.size();
}

size_t DFB::ownerIDFromTileID(size_t tileID) const
{
  return tileID % mpicommon::workerSize();
}

void DFB::createTiles()
{
  size_t tileID = 0;
  vec2i numPixels = getNumPixels();
  for (int y = 0; y < numPixels.y; y += TILE_SIZE) {
    for (int x = 0; x < numPixels.x; x += TILE_SIZE, tileID++) {
      const size_t ownerID = ownerIDFromTileID(tileID);
      const vec2i tileStart(x, y);
      if (ownerID == size_t(mpicommon::workerRank())) {
        auto td = tileOperation->makeTile(this, tileStart, tileID, ownerID);
        myTiles.push_back(td);
        allTiles.push_back(td);
      } else {
        allTiles.push_back(
            std::make_shared<TileDesc>(tileStart, tileID, ownerID));
      }
    }
  }
}

void DFB::setTileOperation(
    std::shared_ptr<TileOperation> tileOp, const Renderer *renderer)
{
  // TODO WILL: Is this really the best way to avoid re-attaching all the
  // tiles each frame? Maybe take a string identifying it and find the
  // obj. creation method instead?
  if (tileOperation && lastRenderer == renderer)
    return;

  tileOperation = tileOp;
  tileOperation->attach(this);
  lastRenderer = renderer;

  allTiles.clear();
  myTiles.clear();
  createTiles();
}

std::shared_ptr<TileOperation> DFB::getTileOperation()
{
  return tileOperation;
}

const Renderer *DFB::getLastRenderer() const
{
  return lastRenderer;
}

const void *DFB::mapBuffer(OSPFrameBufferChannel channel)
{
  if (!localFBonMaster) {
    throw std::runtime_error(
        "#osp:mpi:dfb: tried to 'ospMap()' a frame "
        "buffer that doesn't have a host-side correspondence");
  }
  return localFBonMaster->mapBuffer(channel);
}

void DFB::unmap(const void *mappedMem)
{
  if (!localFBonMaster) {
    throw std::runtime_error(
        "#osp:mpi:dfb: tried to 'ospUnmap()' a frame "
        "buffer that doesn't have a host-side color "
        "buffer");
  }
  localFBonMaster->unmap(mappedMem);
}

void DFB::waitUntilFinished()
{
  using namespace mpicommon;
  using namespace std::chrono;

#ifdef ENABLE_PROFILING
  ProfilingPoint start;
#endif
  std::unique_lock<std::mutex> lock(mutex);
  frameDoneCond.wait(lock, [&] { return frameIsDone; });

  int renderingCancelled = frameCancelled();
  mpicommon::bcast(&renderingCancelled, 1, MPI_INT, masterRank(), mpiGroup.comm)
      .wait();
  frameIsActive = false;

#ifdef ENABLE_PROFILING
  ProfilingPoint end;
  std::cout << "Waiting for completion and sync of cancellation status: "
            << elapsedTimeMs(start, end) << "ms\n";
#endif

  // Report that we're 100% done and do a final check for cancellation
  reportProgress(1.0f);
  setCompletedEvent(OSP_WORLD_RENDERED);

  if (renderingCancelled) {
    return;
  }

#ifdef ENABLE_PROFILING
  start = ProfilingPoint();
#endif
  if (colorBufferFormat != OSP_FB_NONE) {
    gatherFinalTiles();
  } else if (hasVarianceBuffer) {
    gatherFinalErrors();
  }
#ifdef ENABLE_PROFILING
  end = ProfilingPoint();
  std::cout << "Gather final tiles took: " << elapsedTimeMs(start, end)
            << "ms\n";
#endif
}

void DFB::processMessage(WriteTileMessage *msg)
{
  ospray::Tile tile;
  unpackWriteTileMessage(msg, tile, hasNormalBuffer || hasAlbedoBuffer);

  auto *tileDesc = this->getTileDescFor(tile.region.lower);
  LiveTileOperation *td = (LiveTileOperation *)tileDesc;
  td->process(tile);
}

template <typename ColorT>
void DistributedFrameBuffer::processMessage(MasterTileMessage_FB<ColorT> *msg)
{
  if (hasVarianceBuffer) {
    const vec2i tileID = msg->coords / TILE_SIZE;
    if (msg->error < (float)inf)
      tileErrorRegion.update(tileID, msg->error);
  }

  vec2i numPixels = getNumPixels();

  MasterTileMessage_FB_Depth<ColorT> *depth = nullptr;
  if (hasDepthBuffer && msg->command & MASTER_TILE_HAS_DEPTH) {
    depth = reinterpret_cast<MasterTileMessage_FB_Depth<ColorT> *>(msg);
  }

  MasterTileMessage_FB_Depth_Aux<ColorT> *aux = nullptr;
  if (msg->command & MASTER_TILE_HAS_AUX)
    aux = reinterpret_cast<MasterTileMessage_FB_Depth_Aux<ColorT> *>(msg);

  ColorT *color = reinterpret_cast<ColorT *>(localFBonMaster->colorBuffer);
  for (int iy = 0; iy < TILE_SIZE; iy++) {
    int iiy = iy + msg->coords.y;
    if (iiy >= numPixels.y) {
      continue;
    }

    for (int ix = 0; ix < TILE_SIZE; ix++) {
      int iix = ix + msg->coords.x;
      if (iix >= numPixels.x) {
        continue;
      }

      color[iix + iiy * numPixels.x] = msg->color[ix + iy * TILE_SIZE];
      if (depth) {
        localFBonMaster->depthBuffer[iix + iiy * numPixels.x] =
            depth->depth[ix + iy * TILE_SIZE];
      }
      if (aux) {
        if (hasNormalBuffer)
          localFBonMaster->normalBuffer[iix + iiy * numPixels.x] =
              aux->normal[ix + iy * TILE_SIZE];
        if (hasAlbedoBuffer)
          localFBonMaster->albedoBuffer[iix + iiy * numPixels.x] =
              aux->albedo[ix + iy * TILE_SIZE];
      }
    }
  }
}

void DFB::tileIsFinished(LiveTileOperation *tile)
{
  if (!imageOps.empty()) {
    std::for_each(imageOps.begin(),
        imageOps.begin() + firstFrameOperation,
        [&](std::unique_ptr<LiveImageOp> &iop) {
#if 0
          PixelOp *pop = dynamic_cast<PixelOp *>(iop);
          if (pop) {
            // p->postAccum(this, tile);
          }
#endif
          LiveTileOp *top = dynamic_cast<LiveTileOp *>(iop.get());
          if (top) {
            top->process(tile->finished);
          }
          // TODO: For now, frame operations must be last
          // in the pipeline
        });
  }

  // Write the final colors into the color buffer
  // normalize and write final color, and compute error
  if (colorBufferFormat != OSP_FB_NONE) {
    auto DFB_writeTile = &ispc::DFB_writeTile_RGBA32F;
    switch (colorBufferFormat) {
    case OSP_FB_RGBA8:
      DFB_writeTile = &ispc::DFB_writeTile_RGBA8;
      break;
    case OSP_FB_SRGBA:
      DFB_writeTile = &ispc::DFB_writeTile_SRGBA;
      break;
    default:
      break;
    }
    DFB_writeTile((ispc::VaryingTile *)&tile->finished, &tile->color);
  }

  auto msg = [&] {
    MasterTileMessageBuilder msg(colorBufferFormat,
        hasDepthBuffer,
        hasNormalBuffer,
        hasAlbedoBuffer,
        tile->begin,
        tile->error);
    msg.setColor(tile->color);
    msg.setDepth(tile->finished.z);
    msg.setNormal((vec3f *)tile->finished.nx);
    msg.setAlbedo((vec3f *)tile->finished.ar);
    return msg;
  };

  // TODO still send normal & albedo?
  if (colorBufferFormat == OSP_FB_NONE) {
    std::lock_guard<std::mutex> lock(tileErrorsMutex);
    tileIDs.push_back(tile->begin / TILE_SIZE);
    tileErrors.push_back(tile->error);
  } else {
    auto tileMsg = msg().message;
    size_t compressedSize = snappy::MaxCompressedLength(tileMsg->size);
    std::vector<char> compressedTile(compressedSize, 0);
    snappy::RawCompress(reinterpret_cast<char *>(tileMsg->data),
        tileMsg->size,
        compressedTile.data(),
        &compressedSize);

    uint32_t tileOffset;
    {
      std::lock_guard<std::mutex> lock(finalTileBufferMutex);
      tileOffset = nextTileWrite;
      tileBufferOffsets.push_back(tileOffset);
      nextTileWrite += compressedSize;
    }
    std::memcpy(
        &tileGatherBuffer[tileOffset], compressedTile.data(), compressedSize);
  }

  if (isFrameComplete(1)) {
    closeCurrentFrame();
  }
}

void DFB::updateProgress(ProgressMessage *msg)
{
  globalTilesCompletedThisFrame += msg->numCompleted;
  const float progress = globalTilesCompletedThisFrame / (float)getTotalTiles();
  reportProgress(progress);
  if (frameCancelled()) {
    sendCancelRenderingMessage();
  }
}

size_t DFB::numMyTiles() const
{
  return myTiles.size();
}

TileDesc *DFB::getTileDescFor(const vec2i &coords) const
{
  return allTiles[getTileIDof(coords)].get();
}

size_t DFB::getTileIDof(const vec2i &c) const
{
  return (c.x / TILE_SIZE) + (c.y / TILE_SIZE) * numTiles.x;
}

std::string DFB::toString() const
{
  return "ospray::DFB";
}

void DFB::incoming(const std::shared_ptr<mpicommon::Message> &message)
{
  auto *msg = (TileMessage *)message->data;
  /*
  if (msg->command & CANCEL_RENDERING) {
    std::cout << "Rank " << mpicommon::globalRank()
      << " rendering was cancelled\n" << std::flush;
    cancelRendering = true;
    // TODO: We need to mark the frame as done as well here, in case
    // this rank had already finished the frame and was waiting.
    // We also then need to know to discard any additional tile messages
    // we receive until we start the next frame.
    closeCurrentFrame();
    return;
  }
  */

  // Any delayed progress messages are about the previous frame anyways,
  // so we don't care.
  if (!frameIsActive && !(msg->command & PROGRESS_MESSAGE)) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!frameIsActive) {
      // TODO will: probably remove, but test this for now.
      PING;
      throw std::runtime_error(
          "Somehow recieved a tile message when frame inactive!?");
    }
  }

  scheduleProcessing(message);
}

void DFB::scheduleProcessing(const std::shared_ptr<mpicommon::Message> &message)
{
  tasking::schedule([=]() {
    auto *msg = (TileMessage *)message->data;
    if (msg->command & MASTER_WRITE_TILE_I8) {
      throw std::runtime_error("#dfb: master msg should not be scheduled!");
    } else if (msg->command & MASTER_WRITE_TILE_F32) {
      throw std::runtime_error("#dfb: master msg should not be scheduled!");
    } else if (msg->command & WORKER_WRITE_TILE) {
      this->processMessage((WriteTileMessage *)msg);
    } else if (msg->command & PROGRESS_MESSAGE) {
      updateProgress((ProgressMessage *)msg);
    } else {
      throw std::runtime_error("#dfb: unknown tile type processed!");
    }
  });
}

void DFB::gatherFinalTiles()
{
  using namespace mpicommon;
  using namespace std::chrono;

#ifdef ENABLE_PROFILING
  ProfilingPoint preGatherComputeStart;
#endif

  const size_t tileSize = masterMsgSize(
      colorBufferFormat, hasDepthBuffer, hasNormalBuffer, hasAlbedoBuffer);

  const int totalTilesExpected =
      std::accumulate(numTilesExpected.begin(), numTilesExpected.end(), 0);

  std::vector<int> processOffsets;
  if (IamTheMaster()) {
    processOffsets.resize(workerSize(), 0);

    int recvOffset = 0;
    for (int i = 0; i < workerSize(); ++i) {
      processOffsets[i] = recvOffset;
      recvOffset += numTilesExpected[i];
    }
  }

#ifdef ENABLE_PROFILING
  ProfilingPoint startGather;
  std::cout << "Pre-gather took: "
            << elapsedTimeMs(preGatherComputeStart, startGather) << "ms\n";
#endif

  // Each rank sends us the offset lists of the individually compressed tiles
  // in its compressed data buffer
  std::vector<uint32_t> compressedTileOffsets;
  if (IamTheMaster()) {
    compressedTileOffsets.resize(totalTilesExpected, 0);
  }
  auto tileOffsetsGather = gatherv(tileBufferOffsets.data(),
      tileBufferOffsets.size(),
      MPI_INT,
      compressedTileOffsets.data(),
      numTilesExpected,
      processOffsets,
      MPI_INT,
      masterRank(),
      mpiGroup.comm);

  // We've got to use an int since Gatherv only takes int counts.
  // However, it's pretty unlikely we'll reach the point where someone
  // is sending 2GB in final tile data from a single process
  const int sendCompressedSize = static_cast<int>(nextTileWrite);
  // Get info about how many bytes each proc is sending us
  std::vector<int> gatherSizes;
  if (IamTheMaster()) {
    gatherSizes.resize(workerSize(), 0);
  }
  gather(&sendCompressedSize,
      1,
      MPI_INT,
      gatherSizes.data(),
      1,
      MPI_INT,
      masterRank(),
      mpiGroup.comm)
      .wait();

  std::vector<int> compressedOffsets;
  if (IamTheMaster()) {
    compressedOffsets.resize(workerSize(), 0);
    int offset = 0;
    for (size_t i = 0; i < gatherSizes.size(); ++i) {
      compressedOffsets[i] = offset;
      offset += gatherSizes[i];
    }
    compressedResults.resize(offset, 0);
  }
  gatherv(tileGatherBuffer.data(),
      sendCompressedSize,
      MPI_BYTE,
      compressedResults.data(),
      gatherSizes,
      compressedOffsets,
      MPI_BYTE,
      masterRank(),
      mpiGroup.comm)
      .wait();

  tileOffsetsGather.wait();

#ifdef ENABLE_PROFILING
  ProfilingPoint endGather;
  std::cout << "Gather time: " << elapsedTimeMs(startGather, endGather)
            << "ms\n";
#endif

  // Now we must decompress each ranks data to process it, though we
  // already know how much data each is sending us and where to write it.
  if (IamTheMaster()) {
#ifdef ENABLE_PROFILING
    ProfilingPoint startFbWrite;
#endif
    tasking::parallel_for(workerSize(), [&](int i) {
      tasking::parallel_for(numTilesExpected[i], [&](int tid) {
        int processTileOffset = processOffsets[i] + tid;
        int compressedSize = 0;
        if (tid + 1 < numTilesExpected[i]) {
          compressedSize = compressedTileOffsets[processTileOffset + 1]
              - compressedTileOffsets[processTileOffset];
        } else {
          compressedSize =
              gatherSizes[i] - compressedTileOffsets[processTileOffset];
        }
        char *decompressedTile = STACK_BUFFER(char, tileSize);
        snappy::RawUncompress(&compressedResults[compressedOffsets[i]
                                  + compressedTileOffsets[processTileOffset]],
            compressedSize,
            decompressedTile);

        auto *msg = reinterpret_cast<TileMessage *>(decompressedTile);
        if (msg->command & MASTER_WRITE_TILE_I8) {
          this->processMessage((MasterTileMessage_RGBA_I8 *)msg);
        } else if (msg->command & MASTER_WRITE_TILE_F32) {
          this->processMessage((MasterTileMessage_RGBA_F32 *)msg);
        } else {
          throw std::runtime_error("#dfb: non-master tile in final gather!");
        }
      });
    });
#ifdef ENABLE_PROFILING
    ProfilingPoint endFbWrite;
    std::cout << "Master tile write time: "
              << elapsedTimeMs(startFbWrite, endFbWrite) << "ms\n";
#endif
  }
}

void DFB::gatherFinalErrors()
{
  using namespace mpicommon;
  using namespace rkcommon;

  std::vector<int> tilesFromRank(workerSize(), 0);
  const int myTileCount = tileIDs.size();
  gather(&myTileCount,
      1,
      MPI_INT,
      tilesFromRank.data(),
      1,
      MPI_INT,
      masterRank(),
      mpiGroup.comm)
      .wait();

  std::vector<char> tileGatherResult;
  std::vector<int> tileBytesExpected(workerSize(), 0);
  std::vector<int> processOffsets(workerSize(), 0);
  const size_t tileInfoSize = sizeof(float) + sizeof(vec2i);
  if (IamTheMaster()) {
    size_t recvOffset = 0;
    for (int i = 0; i < workerSize(); ++i) {
      processOffsets[i] = recvOffset;
      tileBytesExpected[i] = tilesFromRank[i] * tileInfoSize;
      recvOffset += tileBytesExpected[i];
    }
    tileGatherResult.resize(recvOffset);
  }

  std::vector<char> sendBuffer(myTileCount * tileInfoSize);
  std::memcpy(
      sendBuffer.data(), tileIDs.data(), tileIDs.size() * sizeof(vec2i));
  std::memcpy(sendBuffer.data() + tileIDs.size() * sizeof(vec2i),
      tileErrors.data(),
      tileErrors.size() * sizeof(float));

  gatherv(sendBuffer.data(),
      sendBuffer.size(),
      MPI_BYTE,
      tileGatherResult.data(),
      tileBytesExpected,
      processOffsets,
      MPI_BYTE,
      masterRank(),
      mpiGroup.comm)
      .wait();

  if (IamTheMaster()) {
    tasking::parallel_for(workerSize(), [&](int rank) {
      const vec2i *tileID = reinterpret_cast<vec2i *>(
          tileGatherResult.data() + processOffsets[rank]);
      const float *error = reinterpret_cast<float *>(tileGatherResult.data()
          + processOffsets[rank] + tilesFromRank[rank] * sizeof(vec2i));
      for (int i = 0; i < tilesFromRank[rank]; ++i) {
        if (error[i] < (float)inf) {
          tileErrorRegion.update(tileID[i], error[i]);
        }
      }
    });
  }
}

void DFB::sendCancelRenderingMessage()
{
  // WILL: Because we don't have a threaded MPI and the master
  // in replicated rendering will just be waiting in the bcast when
  // we call cancel, the message would never actually be sent out this
  // frame. It would actually be delayed and sent the following frame.
  // Instead, the best we can do without a threaded MPI at this point
  // is to just cancel the final gathering stage.
  cancelFrame();
  /*
  std::cout << "SENDING CANCEL RENDERING MSG\n";
  PING;
  std::cout << std::flush;
  auto msg = std::make_shared<mpicommon::Message>(sizeof(TileMessage));

  auto out = msg->data;
  int val = CANCEL_RENDERING;
  memcpy(out, &val, sizeof(val));

  for (int rank = 0; rank < mpicommon::numGlobalRanks(); rank++)
    mpi::messaging::sendTo(rank, myId, msg);
  */
}

void DFB::closeCurrentFrame()
{
  std::lock_guard<std::mutex> lock(mutex);
  frameIsDone = true;
  frameDoneCond.notify_all();
}

//! write given tile data into the frame buffer, sending to remote owner if
//! required
void DFB::setTile(ospray::Tile &tile)
{
  auto *tileDesc = this->getTileDescFor(tile.region.lower);

  // Note my tile, send to the owner
  if (!tileDesc->mine()) {
    auto msg = makeWriteTileMessage(tile, hasNormalBuffer || hasAlbedoBuffer);

    int dstRank = tileDesc->ownerID;
    mpi::messaging::sendTo(dstRank, myId, msg);
  } else {
    if (!frameIsActive)
      throw std::runtime_error("#dfb: cannot setTile if frame is inactive!");

    LiveTileOperation *td = (LiveTileOperation *)tileDesc;
    td->process(tile);
  }
}

void DFB::clear()
{
  frameID = -1; // we increment at the start of the frame
  std::fill(tileAccumID.begin(), tileAccumID.end(), 0);

  if (hasAccumBuffer) {
    tileErrorRegion.clear();
  }
}

int32 DFB::accumID(const vec2i &tile)
{
  if (!hasAccumBuffer) {
    return 0;
  }
  return tileAccumID[tile.y * numTiles.x + tile.x];
}

float DFB::tileError(const vec2i &tile)
{
  return tileErrorRegion[tile];
}

void DFB::endFrame(const float errorThreshold, const Camera *camera)
{
  // TODO: FrameOperations should just be run on the master process for now,
  // but in the offload device the master process doesn't get any OSPData
  // or pixel ops or etc. created, just the handles. So it doesn't even
  // know about the frame operation to run
  if (localFBonMaster && !imageOps.empty()
      && firstFrameOperation < imageOps.size()) {
    std::for_each(imageOps.begin() + firstFrameOperation,
        imageOps.end(),
        [&](std::unique_ptr<LiveImageOp> &iop) {
          LiveFrameOp *fop = dynamic_cast<LiveFrameOp *>(iop.get());
          if (fop)
            fop->process(camera);
        });
  }

  std::for_each(imageOps.begin(),
      imageOps.end(),
      [](std::unique_ptr<LiveImageOp> &p) { p->endFrame(); });

  // only refine on master
  if (mpicommon::IamTheMaster()) {
    frameVariance = tileErrorRegion.refine(errorThreshold);
  }

  if (hasAccumBuffer) {
    std::transform(tileAccumID.begin(),
        tileAccumID.end(),
        tileAccumID.begin(),
        [](const uint32_t &x) { return x + 1; });
  }

  setCompletedEvent(OSP_FRAME_FINISHED);
}

} // namespace ospray
