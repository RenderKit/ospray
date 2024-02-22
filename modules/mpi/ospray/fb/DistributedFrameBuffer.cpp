// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedFrameBuffer.h"
#include "DistributedFrameBuffer_TileMessages.h"
#include "ISPCDevice.h"
#include "TileOperation.h"
#include "fb/FrameBufferView.h"
#include "rkcommon/tracing/Tracing.h"
#ifndef OSPRAY_TARGET_SYCL
#include "fb/DistributedFrameBuffer_ispc.h"
#endif

#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/tasking/schedule.h"

#include "api/Device.h"
#include "common/Collectives.h"
#include "common/MPICommon.h"

#include <snappy.h>
#include <numeric>
#include <thread>

using namespace std::chrono;

namespace ospray {

// Helper types /////////////////////////////////////////////////////////////

using DFB = DistributedFrameBuffer;

// DistributedTileError definitions /////////////////////////////////////////

DistributedTileError::DistributedTileError(
    api::ISPCDevice &device, const vec2i &numTiles, mpicommon::Group group)
    : TaskError(device.getIspcrtContext(), numTiles), group(group)
{}

void DistributedTileError::sync()
{
  if (!taskErrorBuffer || taskErrorBuffer->size() == 0) {
    return;
  }

  // TODO: USM thrashing possible issue
  mpicommon::bcast(taskErrorBuffer->data(),
      taskErrorBuffer->size(),
      MPI_FLOAT,
      0,
      group.comm)
      .wait();
}

// DistributedFrameBuffer definitions ///////////////////////////////////////

DFB::DistributedFrameBuffer(api::ISPCDevice &device,
    const vec2i &numPixels,
    ObjectHandle myId,
    ColorBufferFormat colorBufferFormat,
    const uint32 channels)
    // TODO WILL: The DFB should use a separate identifier than myID
    // since large scenes can push this ID beyond the max value we can use
    // for MPI Tag (i.e., Moana). The IDs for the message handler should not
    // be set from the object handle but pulled from some other ID pool
    // specific to those objects using the messaging layer
    : MessageHandler(myId),
      FrameBuffer(device, numPixels, colorBufferFormat, channels, FFO_NONE),
      mpiGroup(mpicommon::worker.dup()),
      totalTiles(divRoundUp(size, vec2i(TILE_SIZE))),
      numRenderTasks((totalTiles * TILE_SIZE) / getRenderTaskSize()),
      tileErrorRegion(
          device, hasVarianceBuffer ? totalTiles : vec2i(0), mpiGroup),
      localFBonMaster(nullptr),
      frameIsActive(false),
      frameIsDone(false)
{
  if (mpicommon::IamTheMaster() && colorBufferFormat != OSP_FB_NONE) {
    localFBonMaster = rkcommon::make_unique<LocalFrameBuffer>(device,
        numPixels,
        colorBufferFormat,
        channels & ~(OSP_FB_ACCUM | OSP_FB_VARIANCE));
  }
}

DFB::~DistributedFrameBuffer()
{
  MPI_Comm_free(&mpiGroup.comm);
}

void DFB::commit()
{
  FrameBuffer::commit();

  if (localFBonMaster) {
    // We need image operations to be added to inner local FB as well, the local
    // FB possess complete image and is responsible for post-processing
    // execution
    if (hasParam("imageOperation"))
      localFBonMaster->setParam("imageOperation",
          static_cast<ManagedObject *>(getParamObject<Data>("imageOperation")));
    localFBonMaster->commit();
  }
}

mpicommon::Group DFB::getMPIGroup()
{
  return mpiGroup;
}

void DFB::startNewFrame(const float errorThreshold)
{
  {
    std::lock_guard<std::mutex> lock(finalTileBufferMutex);
    nextTileWrite = 0;
    tileBufferOffsets.clear();
  }
  if (colorBufferFormat != OSP_FB_NONE) {
    RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::beginEvent(
        "startFrame::reserveTileGatherBuf", "DFB"));
    // Allocate a conservative upper bound of space which we'd need to
    // store the compressed tiles
    const size_t uncompressedSize = masterMsgSize(
        hasDepthBuffer, hasNormalBuffer, hasAlbedoBuffer, hasIDBuf());
    const size_t compressedSize = snappy::MaxCompressedLength(uncompressedSize);
    tileGatherBuffer.resize(myTiles.size() * compressedSize, 0);
    RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());
  }

  RKCOMMON_IF_TRACING_ENABLED(
      rkcommon::tracing::beginEvent("startFrame::initTilesInfo", "DFB"));
  {
    std::lock_guard<std::mutex> fbLock(mutex);
    std::lock_guard<std::mutex> numTilesLock(numTilesMutex);

    if (frameIsActive) {
      throw std::runtime_error(
          "Attempt to start frame on already started frame!");
    }

    frameProgress = 0.f;

    FrameBuffer::beginFrame();

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

    for (auto &l : layers) {
      l->beginFrame();
    }

    if (mpicommon::IamTheMaster()) {
      numTilesExpected.resize(mpicommon::workerSize(), 0);
      std::fill(numTilesExpected.begin(), numTilesExpected.end(), 0);
    }

    globalTilesCompletedThisFrame = 0;
    numTilesCompletedThisFrame = 0;
    for (uint32_t t = 0; t < getGlobalTotalTiles(); t++) {
      if (doAccumulation() && tileError(t) <= errorThreshold) {
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
  RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());
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
    msgData->frameID = getSh()->frameID;
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

void DistributedFrameBuffer::setSparseFBLayerCount(size_t numLayers)
{
  // Layer 0 is the base owned tiles layer created by the DFB, so we'll always
  // have numLayers + 1 layers
  layers.resize(numLayers + 1, nullptr);
  // The sparse layers don't do accumulation and variance calculation because
  // this needs data from all ranks rendering a given tile
  const int channelFlags =
      getChannelFlags() & ~(OSP_FB_ACCUM | OSP_FB_VARIANCE);
  // Allocate any new layers that have been added to the DFB
  for (size_t i = 1; i < layers.size(); ++i) {
    if (!layers[i]) {
      layers[i] = rkcommon::make_unique<SparseFrameBuffer>(
          getISPCDevice(), size, getColorBufferFormat(), channelFlags);
    }
  }
}

size_t DistributedFrameBuffer::getSparseLayerCount() const
{
  return layers.size();
}

SparseFrameBuffer *DistributedFrameBuffer::getSparseFBLayer(size_t l)
{
  return layers[l].get();
}

void DFB::cancelFrame()
{
  FrameBuffer::cancelFrame();

  // Propagate the frame cancellation to the sparse fb layers as well
  for (auto &l : layers) {
    l->cancelFrame();
  }
}

float DFB::getCurrentProgress() const
{
  if (stagesCompleted == OSP_FRAME_FINISHED) {
    return 1.f;
  }
  return frameProgress;
}

void DFB::createTiles()
{
  allTiles.clear();
  myTiles.clear();

  const vec2i totalTiles = divRoundUp(size, vec2i(TILE_SIZE));
  std::vector<uint32_t> myTileIDs;
  for (int y = 0; y < totalTiles.y; ++y) {
    for (int x = 0; x < totalTiles.x; ++x) {
      size_t tileID = size_t(x) + size_t(totalTiles.x) * size_t(y);
      const size_t ownerID = ownerIDFromTileID(tileID);
      const vec2i tileStart(x * TILE_SIZE, y * TILE_SIZE);
      if (ownerID == size_t(mpicommon::workerRank())) {
        auto td = tileOperation->makeTile(this, tileStart, tileID, ownerID);
        myTiles.push_back(td.get());
        allTiles.push_back(std::move(td));
        myTileIDs.push_back(tileID);
      } else {
        allTiles.push_back(make_unique<TileDesc>(tileStart, tileID, ownerID));
      }
    }
  }

  // Allocate the sparse fb for the tiles we own
  uint32 channels = OSP_FB_DEPTH;
  // Accum and variance are not done in the sparse FB for the DFB
  if (hasNormalBuf()) {
    channels |= OSP_FB_NORMAL;
  }
  if (hasAlbedoBuf()) {
    channels |= OSP_FB_ALBEDO;
  }

  if (layers.empty()) {
    layers.push_back(rkcommon::make_unique<SparseFrameBuffer>(
        getISPCDevice(), size, OSP_FB_NONE, channels, myTileIDs));
  } else {
    layers[0]->setTiles(myTileIDs);
    layers[0]->clear();
  }
  layers[0]->commit();
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

  RKCOMMON_IF_TRACING_ENABLED(
      rkcommon::tracing::beginEvent("waitUntilDoneCond", "DFB"));
  std::unique_lock<std::mutex> lock(mutex);
  frameDoneCond.wait(lock, [&] { return frameIsDone; });

  frameIsActive = false;

  RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());

  setCompletedEvent(OSP_WORLD_RENDERED);

  if (frameCancelled()) {
    return;
  }

  RKCOMMON_IF_TRACING_ENABLED(
      rkcommon::tracing::beginEvent("finalGather", "DFB"));
  if (colorBufferFormat != OSP_FB_NONE) {
    gatherFinalTiles();
  } else if (hasVarianceBuf()) {
    gatherFinalErrors();
  }
  RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());
}

void DFB::processMessage(WriteTileMessage *msg)
{
  ispc::Tile tile;
  unpackWriteTileMessage(
      msg, tile, hasNormalBuffer || hasAlbedoBuffer || hasIDBuf());

  auto *tileDesc = this->getTileDescFor(tile.region.lower);
  LiveTileOperation *td = (LiveTileOperation *)tileDesc;
  td->process(tile);
}

void DistributedFrameBuffer::processMessage(MasterTileMessage_FB *msg)
{
  if (hasVarianceBuffer) {
    const vec2i tileID = msg->coords / TILE_SIZE;
    if (msg->error < (float)inf) {
      tileErrorRegion.update(tileID, msg->error);
    }
  }

  vec2i numPixels = getNumPixels();

  MasterTileMessage_FB_Depth *depth = nullptr;
  if (hasDepthBuffer && msg->command & MASTER_TILE_HAS_DEPTH) {
    depth = reinterpret_cast<MasterTileMessage_FB_Depth *>(msg);
  }

  MasterTileMessage_FB_Depth_Aux *aux = nullptr;
  if (msg->command & MASTER_TILE_HAS_AUX)
    aux = reinterpret_cast<MasterTileMessage_FB_Depth_Aux *>(msg);

  uint32 *pidBuf = nullptr;
  uint32 *gidBuf = nullptr;
  uint32 *iidBuf = nullptr;
  // TODO: Make the rest of the tile more dynamically sized and use a buffer
  // cursor style to get the pointers to the individual tile components
  if (msg->command & MASTER_TILE_HAS_ID) {
    // The ID buffer data comes at the end of the tile message
    uint8 *data = reinterpret_cast<uint8 *>(msg);
    if (!aux) {
      if (depth) {
        data += sizeof(MasterTileMessage_FB_Depth);
      } else {
        data += sizeof(MasterTileMessage_FB);
      }
    } else {
      data += sizeof(MasterTileMessage_FB_Depth_Aux);
    }
    // All IDs are sent if any were requested, however we need to write only the
    // ones that actually exist in the local FB since it doesn't allocate
    // buffers for the non-existant channels
    if (hasPrimitiveIDBuffer) {
      pidBuf = reinterpret_cast<uint32 *>(data);
    }
    if (hasObjectIDBuffer) {
      gidBuf = reinterpret_cast<uint32 *>(
          data + sizeof(uint32) * TILE_SIZE * TILE_SIZE);
    }
    if (hasInstanceIDBuffer) {
      iidBuf = reinterpret_cast<uint32 *>(
          data + 2 * sizeof(uint32) * TILE_SIZE * TILE_SIZE);
    }
  }

  // TODO: Here we're just accessing the local fb on the host, but have it
  // allocated in USM. Will work, but maybe wasting some USM space?
  vec4f *color = (localFBonMaster->colorBuffer)
      ? localFBonMaster->colorBuffer->data()
      : nullptr;
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

      if (color) {
        color[iix + iiy * numPixels.x] = msg->color[ix + iy * TILE_SIZE];
      }
      if (depth) {
        (*localFBonMaster->depthBuffer)[iix + iiy * numPixels.x] =
            depth->depth[ix + iy * TILE_SIZE];
      }
      if (aux) {
        if (hasNormalBuffer)
          (*localFBonMaster->normalBuffer)[iix + iiy * numPixels.x] =
              aux->normal[ix + iy * TILE_SIZE];
        if (hasAlbedoBuffer)
          (*localFBonMaster->albedoBuffer)[iix + iiy * numPixels.x] =
              aux->albedo[ix + iy * TILE_SIZE];
      }
      if (pidBuf) {
        (*localFBonMaster->primitiveIDBuffer)[iix + iiy * numPixels.x] =
            pidBuf[ix + iy * TILE_SIZE];
      }
      if (gidBuf) {
        (*localFBonMaster->objectIDBuffer)[iix + iiy * numPixels.x] =
            gidBuf[ix + iy * TILE_SIZE];
      }
      if (iidBuf) {
        (*localFBonMaster->instanceIDBuffer)[iix + iiy * numPixels.x] =
            iidBuf[ix + iy * TILE_SIZE];
      }
    }
  }
}

bool DFB::hasIDBuf() const
{
  // ID buffers are only needed on the first frame when rendering with
  // accumulation
  return getFrameID() == 0
      && (hasPrimitiveIDBuffer || hasObjectIDBuffer || hasInstanceIDBuffer);
}

void DFB::tileIsFinished(LiveTileOperation *tile)
{
  // Write the final colors into the color buffer
  // normalize and write final color, and compute error
  if (colorBufferFormat != OSP_FB_NONE)
    DFB_writeTile((ispc::VaryingTile *)&tile->finished, &tile->color);

  auto msg = [&] {
    MasterTileMessageBuilder msg(hasDepthBuffer,
        hasNormalBuffer,
        hasAlbedoBuffer,
        hasIDBuf(),
        tile->begin,
        tile->error);
    msg.setColor(tile->color);
    msg.setDepth(tile->finished.z);
    msg.setNormal((vec3f *)tile->finished.nx);
    msg.setAlbedo((vec3f *)tile->finished.ar);
    msg.setPrimitiveID(tile->finished.pid);
    msg.setObjectID(tile->finished.gid);
    msg.setInstanceID(tile->finished.iid);
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
  frameProgress = globalTilesCompletedThisFrame / (float)getGlobalTotalTiles();
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
  return (c.x / TILE_SIZE) + (c.y / TILE_SIZE) * totalTiles.x;
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
      throw std::runtime_error(
          "Somehow received a tile message when frame inactive!?");
    }
  }

  scheduleProcessing(message);
}

void DFB::scheduleProcessing(const std::shared_ptr<mpicommon::Message> &message)
{
  tasking::schedule([=]() {
    auto *msg = (TileMessage *)message->data;
    if (msg->command & MASTER_WRITE_TILE) {
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

  RKCOMMON_IF_TRACING_ENABLED(
      rkcommon::tracing::beginEvent("preGatherComputeStart", "DFB"));

  const size_t tileSize = masterMsgSize(
      hasDepthBuffer, hasNormalBuffer, hasAlbedoBuffer, hasIDBuf());

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

  RKCOMMON_IF_TRACING_ENABLED({
    rkcommon::tracing::endEvent();
    rkcommon::tracing::beginEvent("gatherTileData", "DFB");
  });

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

  RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());

  // Now we must decompress each ranks data to process it, though we
  // already know how much data each is sending us and where to write it.
  if (IamTheMaster()) {
    RKCOMMON_IF_TRACING_ENABLED(
        rkcommon::tracing::beginEvent("masterTileWrite", "DFB"));
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

        MasterTileMessage_FB *msg =
            reinterpret_cast<MasterTileMessage_FB *>(decompressedTile);
        this->processMessage(msg);
      });
    });
    RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());
    // not accurate, did we render something at all
    localFBonMaster->getSh()->super.numPixelsRendered = totalTilesExpected;
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

void DFB::closeCurrentFrame()
{
  std::lock_guard<std::mutex> lock(mutex);
  frameIsDone = true;
  frameDoneCond.notify_all();
}

//! write given tile data into the frame buffer, sending to remote owner if
//! required
void DFB::setTile(const ispc::Tile &tile)
{
  auto *tileDesc = this->getTileDescFor(tile.region.lower);

  // Note my tile, send to the owner
  if (!tileDesc->mine()) {
    auto msg = makeWriteTileMessage(
        tile, hasNormalBuffer || hasAlbedoBuffer || hasIDBuf());

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
  FrameBuffer::clear();

  if (hasVarianceBuffer) {
    tileErrorRegion.clear();
  }
  if (localFBonMaster) {
    localFBonMaster->clear();
  }
  for (auto &l : layers) {
    l->clear();
  }
}

vec2i DFB::getNumRenderTasks() const
{
  return numRenderTasks;
}

uint32_t DFB::getTotalRenderTasks() const
{
  return numRenderTasks.product();
}

vec2i DFB::getGlobalNumTiles() const
{
  return totalTiles;
}

uint32_t DFB::getGlobalTotalTiles() const
{
  return totalTiles.product();
}

utility::ArrayView<uint32_t> DFB::getRenderTaskIDs(
    const float errorThreshold, const uint32_t spp)
{
  return layers[0]->getRenderTaskIDs(errorThreshold, spp);
}

float DFB::getVariance() const
{
  // TODO: No proper error calculation for DFB now,
  // need to use FrameOp via LocalFB
  if (mpicommon::IamTheMaster())
    return 1.f;

  // Return set value otherwise
  return FrameBuffer::getVariance();
}

float DFB::taskError(const uint32_t) const
{
  NOT_IMPLEMENTED;
}

float DFB::tileError(const uint32_t tileID)
{
  return tileErrorRegion[tileID];
}

AsyncEvent DFB::postProcess(bool wait)
{
  AsyncEvent event;
  if (localFBonMaster) {
    // FrameOps are run on the device, but the DFB receives the final image
    // data over MPI into host-memory, and returns host-memory pointers
    // directly. So, we need to copy the host data to the device, run the frame
    // ops. If we can receive with MPI directly into device
    // memory we can drop this first copy step. When running on the CPU device,
    // these copies will become no-ops.
    ispcrt::TaskQueue &tq = getISPCDevice().getIspcrtQueue();
    if (localFBonMaster->colorBuffer) {
      tq.copyToDevice(*localFBonMaster->colorBuffer);
    }
    if (localFBonMaster->depthBuffer) {
      tq.copyToDevice(*localFBonMaster->depthBuffer);
    }
    if (localFBonMaster->normalBuffer) {
      tq.copyToDevice(*localFBonMaster->normalBuffer);
    }
    if (localFBonMaster->albedoBuffer) {
      tq.copyToDevice(*localFBonMaster->albedoBuffer);
    }
    tq.sync();
    event = localFBonMaster->postProcess(wait);
  }
  return event;
}

} // namespace ospray
