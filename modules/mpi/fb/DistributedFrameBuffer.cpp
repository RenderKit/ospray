// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include <thread>
#include <snappy.h>
#include "DistributedFrameBuffer.h"
#include "DistributedFrameBuffer_TileTypes.h"
#include "DistributedFrameBuffer_TileMessages.h"
#include "DistributedFrameBuffer_ispc.h"

#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/tasking/schedule.h"
#include "pico_bench.h"

#include "mpiCommon/MPICommon.h"
#include "api/Device.h"

#ifdef _WIN32
#  include <windows.h> // for Sleep
#endif

#if 0
# define DBG(a) a
#else
# define DBG(a) /* ignore */
#endif

using std::cout;
using std::endl;
using namespace std::chrono;

namespace ospray {

  // Helper types /////////////////////////////////////////////////////////////

  using DFB = DistributedFrameBuffer;

  // DistributedTileError definitions /////////////////////////////////////////

  DistributedTileError::DistributedTileError(const vec2i &numTiles)
    : TileError(numTiles)
  {
  }

  void DistributedTileError::sync()
  {
    if (tiles <= 0)
      return;

    MPI_CALL(Bcast(tileErrorBuffer, tiles, MPI_FLOAT, 0, mpicommon::world.comm));
  }

  // DistributedFrameBuffer definitions ///////////////////////////////////////

  DFB::DistributedFrameBuffer(const vec2i &numPixels,
                              ObjectHandle myId,
                              ColorBufferFormat colorBufferFormat,
                              const uint32 channels,
                              bool masterIsAWorker)
    : MessageHandler(myId),
      FrameBuffer(numPixels, colorBufferFormat, channels),
      tileErrorRegion(hasVarianceBuffer ? getNumTiles() : vec2i(0)),
      localFBonMaster(nullptr),
      frameMode(WRITE_MULTIPLE),
      frameIsActive(false),
      frameIsDone(false),
      masterIsAWorker(masterIsAWorker)
  {
    this->ispcEquivalent = ispc::DFB_create(this);
    ispc::DFB_set(getIE(), numPixels.x, numPixels.y, colorBufferFormat);

    createTiles();

    // TODO: accumID is eventually only needed on master once static
    // loadbalancing is removed
    const size_t bytes = sizeof(int32)*getTotalTiles();
    tileAccumID = (int32*)alignedMalloc(bytes);
    memset(tileAccumID, 0, bytes);

    tileInstances = (int32*)alignedMalloc(bytes);
    memset(tileInstances, 0, bytes);

    if (mpicommon::IamTheMaster()) {
      if (colorBufferFormat == OSP_FB_NONE) {
        DBG(cout << "#osp:mpi:dfb: we're the master, but framebuffer has 'NONE' "
                 << "format; creating distributed frame buffer WITHOUT having a "
                 << "mappable copy on the master" << endl);
      } else {
        localFBonMaster
          = ospcommon::make_unique<LocalFrameBuffer>(numPixels,
              colorBufferFormat,
              channels & ~(OSP_FB_ACCUM | OSP_FB_VARIANCE));
      }
    }
  }

  DFB::~DistributedFrameBuffer()
  {
    freeTiles();
    alignedFree(tileAccumID);
    alignedFree(tileInstances);
  }

  void DFB::startNewFrame(const float errorThreshold)
  {
    queueTimes.clear();
    workTimes.clear();

    nextTileWrite = 0;
    if (colorBufferFormat != OSP_FB_NONE) {
      const size_t finalTileSize = masterMsgSize(colorBufferFormat,
                                                 hasDepthBuffer,
                                                 hasNormalBuffer,
                                                 hasAlbedoBuffer);
      tileGatherBuffer.resize(myTiles.size() * finalTileSize, 0);
      std::fill(tileGatherBuffer.begin(), tileGatherBuffer.end(), 0);
    }

    std::vector<std::shared_ptr<mpicommon::Message>> _delayedMessage;
    {
      SCOPED_LOCK(mutex);
      std::lock_guard<std::mutex> numTilesLock(numTilesMutex);

      DBG(printf("rank %i starting new frame\n", mpicommon::globalRank()));
      assert(!frameIsActive);
      if (frameIsActive) {
        throw std::runtime_error("Attempt to start frame on already started frame!");
      }

      if (pixelOp)
        pixelOp->beginFrame();

      lastProgressReport = std::chrono::high_resolution_clock::now();
      renderingProgressTiles = 0;

      // create a local copy of delayed tiles, so we can work on them outside
      // the mutex
      _delayedMessage = this->delayedMessage;
      this->delayedMessage.clear();

      // NOTE: Doing error sync may do a broadcast, needs to be done before
      //       async messaging enabled in beginFrame()
      tileErrorRegion.sync();
      // TODO WILL: Why is this needed? All ranks will know which ranks own
      // which tiles, since it's assigned round-robin.
      MPI_CALL(Bcast(tileInstances, getTotalTiles(), MPI_INT, 0,
                     mpicommon::world.comm));

      if (colorBufferFormat == OSP_FB_NONE) {
        SCOPED_LOCK(tileErrorsMutex);
        tileIDs.clear();
        tileErrors.clear();
        tileIDs.reserve(myTiles.size());
        tileErrors.reserve(myTiles.size());
      }

      // after Bcast of tileInstances (needed in WriteMultipleTile::newFrame)
      for (auto &tile : myTiles)
        tile->newFrame();

      if (mpicommon::IamTheMaster()) {
        numTilesExpected.resize(mpicommon::numGlobalRanks(), 0);
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

      // set frame to active - this HAS TO BE the last thing we do
      // before unlockign the mutex, because the 'incoming()' message
      // will actually NOT lock the mutex when checking if
      // 'frameIsActive' is true: as soon as the frame is tagged active,
      // incoming WILL write into the frame buffer, composite tiles,
      // etc!
      // XXX So, shouldn't this wait until beginFrame()??
      frameIsActive = true;
    }

    for (auto &msg : _delayedMessage) {
      // TODO WILL: I don't think for regular tile messages (which are
      // required to finish the frame) we will ever have delayed messages.
      // We inherently can't complete the frame without them
      scheduleProcessing(msg);
    }

    mpi::messaging::enableAsyncMessaging();

    if (isFrameComplete(0))
      closeCurrentFrame();
  }

  void DFB::freeTiles()
  {
    for (auto &tile : allTiles)
      delete tile;

    allTiles.clear();
    myTiles.clear();
  }

  bool DFB::isFrameComplete(const size_t numTiles)
  {
    SCOPED_LOCK(numTilesMutex);
    numTilesCompletedThisFrame += numTiles;

    if (reportRenderingProgress) {
      renderingProgressTiles += numTiles;

      auto now = std::chrono::high_resolution_clock::now();
      auto timeSinceUpdate = duration_cast<milliseconds>(now - lastProgressReport);
      if (timeSinceUpdate.count() >= 1000) {
        auto msg = std::make_shared<mpicommon::Message>(sizeof(ProgressMessage));
        ProgressMessage *msgData = reinterpret_cast<ProgressMessage*>(msg->data);
        msgData->command = PROGRESS_MESSAGE;
        msgData->numCompleted = renderingProgressTiles;
        msgData->frameID = frameID;
        mpi::messaging::sendTo(mpicommon::masterRank(), myId, msg);

        renderingProgressTiles = 0;
        lastProgressReport = now;
      }
    }

    if (mpicommon::IamAWorker()
        || (mpicommon::IamTheMaster() && masterIsAWorker))
    {
      return numTilesCompletedThisFrame == myTiles.size();
    }
    // Note: This test is not actually going to ever be true on the master,
    // because it doesn't finish tiles until the gather at the end of the frame
    return numTilesCompletedThisFrame == static_cast<size_t>(getTotalTiles());
  }

  size_t DFB::ownerIDFromTileID(size_t tileID) const
  {
    return masterIsAWorker ? tileID % mpicommon::numGlobalRanks() :
      mpicommon::globalRankFromWorkerRank(tileID % mpicommon::numWorkers());
  }

  TileData *DFB::createTile(const vec2i &xy, size_t tileID, size_t ownerID)
  {
    TileData *td = nullptr;

    switch(frameMode) {
    case WRITE_MULTIPLE:
      td = new WriteMultipleTile(this, xy, tileID, ownerID);
      break;
    case ALPHA_BLEND:
      td = new AlphaBlendTile_simple(this, xy, tileID, ownerID);
      break;
    case Z_COMPOSITE:
      size_t numWorkers = masterIsAWorker ? mpicommon::numGlobalRanks() :
                                            mpicommon::numWorkers();
      td = new ZCompositeTile(this, xy, tileID, ownerID, numWorkers);
      break;
    }

    return td;
  }

  void DFB::createTiles()
  {
    size_t tileID = 0;
    vec2i numPixels = getNumPixels();
    for (int y = 0; y < numPixels.y; y += TILE_SIZE) {
      for (int x = 0; x < numPixels.x; x += TILE_SIZE, tileID++) {
        const size_t ownerID = ownerIDFromTileID(tileID);
        const vec2i tileStart(x, y);
        if (ownerID == size_t(mpicommon::globalRank())) {
          TileData *td = createTile(tileStart, tileID, ownerID);
          myTiles.push_back(td);
          allTiles.push_back(td);
        } else {
          allTiles.push_back(new TileDesc(tileStart, tileID, ownerID));
        }
      }
    }
  }

  void DFB::setFrameMode(FrameMode newFrameMode)
  {
    if (frameMode == newFrameMode)
      return;

    freeTiles();
    this->frameMode = newFrameMode;
    createTiles();
  }

  const void *DFB::mapBuffer(OSPFrameBufferChannel channel)
  {
    if (!localFBonMaster) {
      throw std::runtime_error("#osp:mpi:dfb: tried to 'ospMap()' a frame "
                      "buffer that doesn't have a host-side correspondence");
    }
    assert(localFBonMaster);
    return localFBonMaster->mapBuffer(channel);
  }

  void DFB::unmap(const void *mappedMem)
  {
    if (!localFBonMaster) {
      throw std::runtime_error("#osp:mpi:dfb: tried to 'ospUnmap()' a frame "
                               "buffer that doesn't have a host-side color "
                               "buffer");
    }
    assert(localFBonMaster);
    localFBonMaster->unmap(mappedMem);
  }

  void DFB::waitUntilFinished()
  {
    using namespace mpicommon;
    using namespace std::chrono;

    auto startWaitFrame = high_resolution_clock::now();
    std::unique_lock<std::mutex> lock(mutex);
    frameDoneCond.wait(lock, [&]{
      return frameIsDone;
    });

    // Broadcast the rendering cancellation status to the workers
    // First we barrier to sync that this Bcast is actually picked up by the
    // right code path. Otherwise it may match with the command receiving
    // bcasts in the worker loop.
    MPI_Request allFrameFinished;
    {
      auto mpilock = mpicommon::acquireMPILock();
      MPI_CALL(Ibarrier(world.comm, &allFrameFinished));
    }

    int barrierFinished = 0;
    while (!barrierFinished) {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      auto mpilock = mpicommon::acquireMPILock();
      MPI_CALL(Test(&allFrameFinished, &barrierFinished, MPI_STATUS_IGNORE));
    }

    frameIsActive = false;
    mpi::messaging::disableAsyncMessaging();

    auto endWaitFrame = high_resolution_clock::now();
    waitFrameFinishTime = duration_cast<RealMilliseconds>(endWaitFrame - startWaitFrame);

    // Report that we're 100% done and do a final check for cancellation
    if (reportRenderingProgress && !api::currentDevice().reportProgress(1.0)) {
      cancelRendering = true;
    }

    if (reportRenderingProgress) {
      int renderingCancelled = cancelRendering.load();
      MPI_CALL(Bcast(&renderingCancelled, 1, MPI_INT, masterRank(), world.comm));
      if (renderingCancelled) {
        return;
      }
    }

    if (colorBufferFormat != OSP_FB_NONE) {
      gatherFinalTiles();
    } else if (hasVarianceBuffer) {
      gatherFinalErrors();
    }
  }

  void DFB::processMessage(WriteTileMessage *msg)
  {
    ospray::Tile tile;
    unpackWriteTileMessage(msg, tile, hasNormalBuffer || hasAlbedoBuffer);
    if (pixelOp) {
      pixelOp->preAccum(tile);
    }
    auto *tileDesc = this->getTileDescFor(tile.region.lower);
    TileData *td = (TileData*)tileDesc;
    td->process(tile);
  }

  template <typename ColorT>
  void DistributedFrameBuffer::processMessage(MasterTileMessage_FB<ColorT> *msg)
  {
    if (hasVarianceBuffer) {
      const vec2i tileID = msg->coords/TILE_SIZE;
      if (msg->error < (float)inf)
        tileErrorRegion.update(tileID, msg->error);
    }

    vec2i numPixels = getNumPixels();

    MasterTileMessage_FB_Depth<ColorT> *depth = nullptr;
    if (hasDepthBuffer && msg->command & MASTER_TILE_HAS_DEPTH) {
      depth = reinterpret_cast<MasterTileMessage_FB_Depth<ColorT>*>(msg);
    }

    MasterTileMessage_FB_Depth_Aux<ColorT> *aux = nullptr;
    if (msg->command & MASTER_TILE_HAS_AUX)
      aux = reinterpret_cast<MasterTileMessage_FB_Depth_Aux<ColorT>*>(msg);

    ColorT *color = reinterpret_cast<ColorT*>(localFBonMaster->colorBuffer);
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
          localFBonMaster->depthBuffer[iix + iiy * numPixels.x]
            = depth->depth[ix + iy * TILE_SIZE];
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

  void DFB::tileIsCompleted(TileData *tile)
  {
    DBG(printf("rank %i: tilecompleted %i,%i\n",mpicommon::globalRank(),
               tile->begin.x,tile->begin.y));

    if (pixelOp) {
      pixelOp->postAccum(tile->final);
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
      DFB_writeTile((ispc::VaryingTile*)&tile->final, &tile->color);
    }

    auto msg = [&]{
      MasterTileMessageBuilder msg(colorBufferFormat, hasDepthBuffer,
                                   hasNormalBuffer, hasAlbedoBuffer,
                                   tile->begin, tile->error);
      msg.setColor(tile->color);
      msg.setDepth(tile->final.z);
      msg.setNormal((vec3f*)tile->final.nx);
      msg.setAlbedo((vec3f*)tile->final.ar);
      return msg;
    };

    // Note: In the data-distributed device the master will be rendering
    // and completing tiles.
    if (mpicommon::IamAWorker()) {
      // TODO still send normal & albedo
      if (colorBufferFormat == OSP_FB_NONE) {
        SCOPED_LOCK(tileErrorsMutex);
        tileIDs.push_back(tile->begin/TILE_SIZE);
        tileErrors.push_back(tile->error);
      } else {
        auto tileMsg = msg().message;
        const size_t n = nextTileWrite.fetch_add(tileMsg->size);
        std::memcpy(&tileGatherBuffer[n], tileMsg->data, tileMsg->size);
      }

      if (isFrameComplete(1)) {
        closeCurrentFrame();
      }
      DBG(printf("RANK %d MARKING AS COMPLETED %i,%i -> %i/%i\n",
                 mpicommon::globalRank(), tile->begin.x, tile->begin.y,
                 numTilesCompletedThisFrame, myTiles.size()));
    } else {
      // TODO: Better unify the master is worker code path
      if (colorBufferFormat == OSP_FB_NONE) {
        SCOPED_LOCK(tileErrorsMutex);
        tileIDs.push_back(tile->begin/TILE_SIZE);
        tileErrors.push_back(tile->error);
      } else {
        auto tileMsg = msg().message;
        const size_t n = nextTileWrite.fetch_add(tileMsg->size);
        std::memcpy(&tileGatherBuffer[n], tileMsg->data, tileMsg->size);
      }

      if (isFrameComplete(1)) {
        closeCurrentFrame();
      }
    }
  }

  void DFB::updateProgress(ProgressMessage *msg)
  {
    globalTilesCompletedThisFrame += msg->numCompleted;
    const float progress = globalTilesCompletedThisFrame / (float)getTotalTiles();
    if (reportRenderingProgress && !api::currentDevice().reportProgress(progress)) {
      sendCancelRenderingMessage();
    }
  }

  size_t DFB::numMyTiles() const
  {
    return myTiles.size();
  }

  TileDesc *DFB::getTileDescFor(const vec2i &coords) const
  {
    return allTiles[getTileIDof(coords)];
  }

  size_t DFB::getTileIDof(const vec2i &c) const
  {
    return (c.x/TILE_SIZE) + (c.y/TILE_SIZE)*numTiles.x;
  }

  std::string DFB::toString() const
  {
    return "ospray::DFB";
  }

  void DFB::incoming(const std::shared_ptr<mpicommon::Message> &message)
  {
    auto *msg = (TileMessage*)message->data;
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
      SCOPED_LOCK(mutex);
      if (!frameIsActive) {
        // TODO WILL: When will we ever get delayed messages when
        // the rendering is working properly? The start/end of a frame
        // is synchronized so it shouldn't be possible for one rank to have
        // started rendering and finishing tiles before another has even
        // begun the frame.
        //
        // frame is not actually active, yet - put the tile into the
        // delayed processing buffer, and return WITHOUT deleting it.
        delayedMessage.push_back(message);
        return;
      }
    }

    scheduleProcessing(message);
  }

  void DFB::scheduleProcessing(const std::shared_ptr<mpicommon::Message> &message)
  {
    auto queuedTask = high_resolution_clock::now();
    tasking::schedule([=]() {
      auto startedTask = high_resolution_clock::now();
      auto *msg = (TileMessage*)message->data;
      if (msg->command & MASTER_WRITE_TILE_I8) {
        throw std::runtime_error("#dfb: master msg should not be scheduled!");
      } else if (msg->command & MASTER_WRITE_TILE_F32) {
        throw std::runtime_error("#dfb: master msg should not be scheduled!");
      } else if (msg->command & WORKER_WRITE_TILE) {
        this->processMessage((WriteTileMessage*)msg);
      } else if (msg->command & PROGRESS_MESSAGE) {
        updateProgress((ProgressMessage*)msg);
      } else {
        throw std::runtime_error("#dfb: unknown tile type processed!");
      }

      auto finishedTask = high_resolution_clock::now();
      auto queueTime = duration_cast<duration<double, std::milli>>(startedTask - queuedTask);
      auto computeTime = duration_cast<duration<double, std::milli>>(finishedTask - startedTask);

      std::lock_guard<std::mutex> lock(statsMutex);
      queueTimes.push_back(queueTime);
      workTimes.push_back(computeTime);
    });
  }

  void DFB::gatherFinalTiles()
  {
    using namespace mpicommon;
    using namespace std::chrono;

    auto preGatherComputeStart = high_resolution_clock::now();

    const size_t tileSize = masterMsgSize(colorBufferFormat, hasDepthBuffer,
                                          hasNormalBuffer, hasAlbedoBuffer);

    const size_t totalTilesExpected = std::accumulate(numTilesExpected.begin(),
                                                      numTilesExpected.end(),
                                                      0);
    std::vector<int> tileBytesExpected(numGlobalRanks(), 0);
    std::vector<int> processOffsets(numGlobalRanks(), 0);
    if (IamTheMaster()) {
      tileGatherResult.resize(totalTilesExpected * tileSize);
      for (size_t i = 0; i < numTilesExpected.size(); ++i) {
        tileBytesExpected[i] = numTilesExpected[i] * tileSize;
      }
      size_t recvOffset = 0;
      for (int i = 0; i < numGlobalRanks(); ++i) {
#if 0
        std::cout << "Expecting " << tileBytesExpected[i] << " bytes from " << i
          << ", #tiles: " << numTilesExpected[i] << "\n" << std::flush;
#endif
        processOffsets[i] = recvOffset;
        recvOffset += tileBytesExpected[i];
      }
    }

#if 1
    const size_t renderedTileBytes = nextTileWrite.load();
    size_t compressedSize = 0;
    if (renderedTileBytes > 0) {
      auto startCompr = high_resolution_clock::now();
      compressedSize = snappy::MaxCompressedLength(renderedTileBytes);
      compressedBuf.resize(compressedSize, 0);
      snappy::RawCompress(tileGatherBuffer.data(), renderedTileBytes,
          compressedBuf.data(), &compressedSize);
      auto endCompr = high_resolution_clock::now();

      compressTime = duration_cast<RealMilliseconds>(endCompr - startCompr);
      compressedPercent = 100.0 * (static_cast<double>(compressedSize) / renderedTileBytes);
    }

    auto startGather = high_resolution_clock::now();
    preGatherDuration = duration_cast<RealMilliseconds>(startGather - preGatherComputeStart);

    // We've got to use an int since Gatherv only takes int counts.
    // However, it's pretty unlikely we'll reach the point where someone
    // is sending 2GB in final tile data from a single process
    const int sendCompressedSize = static_cast<int>(compressedSize);
    // Get info about how many bytes each proc is sending us
    std::vector<int> gatherSizes(numGlobalRanks(), 0);
    MPI_CALL(Gather(&sendCompressedSize, 1, MPI_INT,
                    gatherSizes.data(), 1, MPI_INT,
                    masterRank(), world.comm));

    std::vector<int> compressedOffsets(numGlobalRanks(), 0);
    int offset = 0;
    for (size_t i = 0; i < gatherSizes.size(); ++i) {
      compressedOffsets[i] = offset;
      offset += gatherSizes[i];
    }

    compressedResults.resize(offset, 0);
    MPI_CALL(Gatherv(compressedBuf.data(), sendCompressedSize, MPI_BYTE,
                     compressedResults.data(), gatherSizes.data(),
                     compressedOffsets.data(), MPI_BYTE,
                     masterRank(), world.comm));
    auto endGather = high_resolution_clock::now();

    if (IamTheMaster()) {
      // Now we must decompress each ranks data to process it, though we
      // already know how much data each is sending us and where to write it.
      auto startCompr = high_resolution_clock::now();
      tasking::parallel_for(numGlobalRanks(), [&](int i) {
          snappy::RawUncompress(&compressedResults[compressedOffsets[i]],
                                gatherSizes[i],
                                &tileGatherResult[processOffsets[i]]);
      });
      auto endCompr = high_resolution_clock::now();
      decompressTime = duration_cast<RealMilliseconds>(endCompr - startCompr);
    }
#else
    auto startGather = high_resolution_clock::now();
    MPI_CALL(Gatherv(tileGatherBuffer.data(), renderedTileBytes, MPI_BYTE,
                     tileGatherResult.data(), tileBytesExpected.data(),
                     processOffsets.data(), MPI_BYTE,
                     masterRank(), world.comm));
    auto endGather = high_resolution_clock::now();
#endif
    finalGatherTime = duration_cast<RealMilliseconds>(endGather - startGather);

    if (IamTheMaster()) {
      auto startMasterWrite = high_resolution_clock::now();
      tasking::parallel_for(totalTilesExpected, [&](size_t tile) {
        auto *msg = reinterpret_cast<TileMessage*>(&tileGatherResult[tile * tileSize]);
        if (msg->command & MASTER_WRITE_TILE_I8) {
          this->processMessage((MasterTileMessage_RGBA_I8*)msg);
        } else if (msg->command & MASTER_WRITE_TILE_F32) {
          this->processMessage((MasterTileMessage_RGBA_F32*)msg);
        } else {
          throw std::runtime_error("#dfb: non-master tile in final gather!");
        }
      });
      auto endMasterWrite = high_resolution_clock::now();
      masterTileWriteTime = duration_cast<RealMilliseconds>(endMasterWrite - startMasterWrite);
    }
  }

  void DFB::gatherFinalErrors()
  {
    using namespace mpicommon;
    using namespace ospcommon;

    std::vector<int> tilesFromRank(numGlobalRanks(), 0);
    const int myTileCount = tileIDs.size();
    MPI_CALL(Gather(&myTileCount, 1, MPI_INT,
                    tilesFromRank.data(), 1, MPI_INT,
                    masterRank(), world.comm));

    std::vector<char> tileGatherResult;
    std::vector<int> tileBytesExpected(numGlobalRanks(), 0);
    std::vector<int> processOffsets(numGlobalRanks(), 0);
    const size_t tileInfoSize = sizeof(float) + sizeof(vec2i);
    if (IamTheMaster()) {
      size_t recvOffset = 0;
      for (int i = 0; i < numGlobalRanks(); ++i) {
        processOffsets[i] = recvOffset;
        tileBytesExpected[i] = tilesFromRank[i] * tileInfoSize;
        recvOffset += tileBytesExpected[i];
      }
      tileGatherResult.resize(recvOffset);
    }

    std::vector<char> sendBuffer(myTileCount * tileInfoSize);
    std::memcpy(sendBuffer.data(), tileIDs.data(), tileIDs.size() * sizeof(vec2i));
    std::memcpy(sendBuffer.data() + tileIDs.size() * sizeof(vec2i),
                tileErrors.data(), tileErrors.size() * sizeof(float));

    MPI_CALL(Gatherv(sendBuffer.data(), sendBuffer.size(), MPI_BYTE,
                     tileGatherResult.data(), tileBytesExpected.data(),
                     processOffsets.data(), MPI_BYTE,
                     masterRank(), world.comm));

    if (IamTheMaster()) {
      tasking::parallel_for(numGlobalRanks(), [&](int rank) {
          const vec2i *tileID = reinterpret_cast<vec2i*>(tileGatherResult.data() + processOffsets[rank]);
          const float *error = reinterpret_cast<float*>(tileGatherResult.data() + processOffsets[rank]
                                                        + tilesFromRank[rank] * sizeof(vec2i));
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
    cancelRendering = true;
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
    DBG(printf("rank %i CLOSES frame\n", mpicommon::globalRank()));

    SCOPED_LOCK(mutex);
    frameIsDone   = true;
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
      DBG(printf("rank %i: send tile %i,%i to %i\n",mpicommon::globalRank(),
                 tileDesc->begin.x,tileDesc->begin.y,dstRank));
      mpi::messaging::sendTo(dstRank, myId, msg);
    } else {
      if (!frameIsActive)
        throw std::runtime_error("#dfb: cannot setTile if frame is inactive!");

      TileData *td = (TileData*)tileDesc;
      td->process(tile);
    }
  }

  /*! \brief clear (the specified channels of) this frame buffer

    \details for the *distributed* frame buffer, we assume that
    *all* nodes get this command, and that each instance therefore
    can clear only its own tiles without having to tell any other
    node about it
  */
  void DFB::clear(const uint32 fbChannelFlags)
  {
    frameID = -1; // we increment at the start of the frame
    if (!myTiles.empty()) {
      tasking::parallel_for(myTiles.size(), [&](size_t taskIndex) {
        TileData *td = this->myTiles[taskIndex];
        assert(td);
        const auto bytes = TILE_SIZE * TILE_SIZE * sizeof(float);
        // XXX needed? DFB_accumulateTile writes when accumId==0
        if (hasAccumBuffer && (fbChannelFlags & OSP_FB_ACCUM)) {
          memset(td->accum.r, 0, bytes);
          memset(td->accum.g, 0, bytes);
          memset(td->accum.b, 0, bytes);
          memset(td->accum.a, 0, bytes);
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->accum.z[i] = inf;
          if (hasVarianceBuffer) { // clearing ACCUM also clears VARIANCE
            memset(td->variance.r, 0, bytes);
            memset(td->variance.g, 0, bytes);
            memset(td->variance.b, 0, bytes);
            memset(td->variance.a, 0, bytes);
          }
        }
        if (hasDepthBuffer && (fbChannelFlags & OSP_FB_DEPTH))
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->final.z[i] = inf;
        if (fbChannelFlags & OSP_FB_COLOR) {
          memset(td->final.r, 0, bytes);
          memset(td->final.g, 0, bytes);
          memset(td->final.b, 0, bytes);
          memset(td->final.a, 0, bytes);
        }
      });
    }

    if (hasAccumBuffer && (fbChannelFlags & OSP_FB_ACCUM)) {
      memset(tileAccumID, 0, getTotalTiles()*sizeof(int32));
      tileErrorRegion.clear();
    }
  }

  int32 DFB::accumID(const vec2i &tile)
  {
    if (!hasAccumBuffer)
      return 0;

    const auto tileNr = tile.y * numTiles.x + tile.x;
    // Note: we increment here and not in endframe because we
    // can render the same tile multiple times with different accumulation
    // IDs when using dynamic load balancing.
    tileInstances[tileNr]++;
    return tileAccumID[tileNr]++;
  }

  float DFB::tileError(const vec2i &tile)
  {
    return tileErrorRegion[tile];
  }

  void DFB::beginFrame()
  {
    cancelRendering = false;
    reportRenderingProgress = api::currentDevice().hasProgressCallback();
    MPI_CALL(Bcast(&reportRenderingProgress, 1, MPI_INT,
                   mpicommon::masterRank(), mpicommon::world.comm));
    FrameBuffer::beginFrame();
  }

  float DFB::endFrame(const float errorThreshold)
  {
    if (mpicommon::IamTheMaster() && !masterIsAWorker) {
      /* do nothing */
    } else {
      if (pixelOp) {
        pixelOp->endFrame();
      }
    }

    memset(tileInstances, 0, sizeof(int32)*getTotalTiles()); // XXX needed?

    if (mpicommon::IamTheMaster()) // only refine on master
      return tileErrorRegion.refine(errorThreshold);
    else // slaves will get updated error with next sync() anyway
      return inf;
  }

  void DFB::reportTimings(std::ostream &os)
  {
#if 1
    std::lock_guard<std::mutex> lock(statsMutex);

    using Stats = pico_bench::Statistics<RealMilliseconds>;
    if (!queueTimes.empty()) {
      Stats queueStats(queueTimes);
      queueStats.time_suffix = "ms";

      os << "Tile Queue times:\n" << queueStats << "\n";
    }

    if (!workTimes.empty()) {
      Stats workStats(workTimes);
      workStats.time_suffix = "ms";
      os << "Tile work times:\n" << workStats << "\n";
    }

    double localWaitTime = finalGatherTime.count();
    os << "Gather time: " << localWaitTime << "ms\n"
      << "Waiting for frame: " << waitFrameFinishTime.count() << "ms\n"
      << "Compress time: " << compressTime.count() << "ms\n"
      << "Compressed buffer size: " << compressedPercent << "%\n"
      << "Pre-gather compute time: " << preGatherDuration.count() << "ms\n";

    double maxWaitTime, minWaitTime;
    MPI_Reduce(&localWaitTime, &maxWaitTime, 1, MPI_DOUBLE,
        MPI_MAX, 0, mpicommon::world.comm);
    MPI_Reduce(&localWaitTime, &minWaitTime, 1, MPI_DOUBLE,
        MPI_MIN, 0, mpicommon::world.comm);

    if (mpicommon::world.rank == 0) {
      os << "Max gather time: " << maxWaitTime << "ms\n"
        << "Min gather time: " << minWaitTime << "ms\n"
        << "Master tile write loop time: " << masterTileWriteTime.count() << "ms\n"
        << "Decompress time: " << decompressTime.count() << "ms\n";
    }
#endif
  }

} // ::ospray

