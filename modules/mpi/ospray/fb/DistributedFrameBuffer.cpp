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
#include "TileOperation.h"
#include "DistributedFrameBuffer_TileMessages.h"
#include "DistributedFrameBuffer_ispc.h"

#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/tasking/schedule.h"
#include "pico_bench.h"

#include "common/MPICommon.h"
#include "common/Collectives.h"
#include "api/Device.h"

using std::cout;
using std::endl;
using namespace std::chrono;

namespace ospray {

  // Helper types /////////////////////////////////////////////////////////////

  using DFB = DistributedFrameBuffer;

  // DistributedTileError definitions /////////////////////////////////////////

  DistributedTileError::DistributedTileError(const vec2i &numTiles,
                                             mpicommon::Group group)
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
      localFBonMaster
        = ospcommon::make_unique<LocalFrameBuffer>(numPixels,
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
                          localFBonMaster ? localFBonMaster->colorBuffer
                                            : nullptr,
                          localFBonMaster ? localFBonMaster->depthBuffer
                                            : nullptr,
                          localFBonMaster ? localFBonMaster->normalBuffer
                                            : nullptr,
                          localFBonMaster ? localFBonMaster->albedoBuffer
                                            : nullptr);

      std::for_each(imageOpData->begin<ImageOp *>(),
                    imageOpData->end<ImageOp *>(),
                    [&](ImageOp *i) {
                      if (!dynamic_cast<FrameOp *>(i) || localFBonMaster)
                        imageOps.push_back(i->attach(fbv));
                    });

      findFirstFrameOperation();
    }
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

    {
      std::lock_guard<std::mutex> fbLock(mutex);
      std::lock_guard<std::mutex> numTilesLock(numTilesMutex);

      if (frameIsActive) {
        throw std::runtime_error("Attempt to start frame on already started frame!");
      }

      std::for_each(imageOps.begin(), imageOps.end(),
                    [](std::unique_ptr<LiveImageOp> &p) { p->beginFrame(); });

      lastProgressReport = std::chrono::high_resolution_clock::now();
      renderingProgressTiles = 0;

      tileErrorRegion.sync();

      if (colorBufferFormat == OSP_FB_NONE) {
        std::lock_guard<std::mutex> errsLock(tileErrorsMutex);
        tileIDs.clear();
        tileErrors.clear();
        tileIDs.reserve(myTiles.size());
        tileErrors.reserve(myTiles.size());
      }

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
    for (int y = 0; y < numPixels.y; y += TILE_SIZE)
    {
      for (int x = 0; x < numPixels.x; x += TILE_SIZE, tileID++)
      {
        const size_t ownerID = ownerIDFromTileID(tileID);
        const vec2i tileStart(x, y);
        if (ownerID == size_t(mpicommon::workerRank())) {
          auto td = tileOperation->makeTile(this, tileStart, tileID, ownerID);
          myTiles.push_back(td);
          allTiles.push_back(td);
        } else {
          allTiles.push_back(std::make_shared<TileDesc>(tileStart,
                                                        tileID,
                                                        ownerID));
        }
      }
    }
  }

  void DFB::setTileOperation(std::shared_ptr<TileOperation> tileOp,
                             const Renderer *renderer)
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

  const Renderer* DFB::getLastRenderer() const
  {
    return lastRenderer;
  }

  const void *DFB::mapBuffer(OSPFrameBufferChannel channel)
  {
    if (!localFBonMaster) {
      throw std::runtime_error("#osp:mpi:dfb: tried to 'ospMap()' a frame "
                      "buffer that doesn't have a host-side correspondence");
    }
    return localFBonMaster->mapBuffer(channel);
  }

  void DFB::unmap(const void *mappedMem)
  {
    if (!localFBonMaster) {
      throw std::runtime_error("#osp:mpi:dfb: tried to 'ospUnmap()' a frame "
                               "buffer that doesn't have a host-side color "
                               "buffer");
    }
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

    int renderingCancelled = frameCancelled();
    mpicommon::bcast(&renderingCancelled, 1, MPI_INT, masterRank(),
                     mpiGroup.comm).wait();
    frameIsActive = false;

    auto endWaitFrame = high_resolution_clock::now();
    waitFrameFinishTime = duration_cast<RealMilliseconds>(endWaitFrame - startWaitFrame);

    // Report that we're 100% done and do a final check for cancellation
    reportProgress(1.0f);
    setCompletedEvent(OSP_WORLD_RENDERED);

    if (renderingCancelled) {
      return;
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

    auto *tileDesc = this->getTileDescFor(tile.region.lower);
    LiveTileOperation *td = (LiveTileOperation*)tileDesc;
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

  void DFB::tileIsFinished(LiveTileOperation *tile)
  {
    if (!imageOps.empty()) {
      std::for_each(imageOps.begin(), imageOps.begin() + firstFrameOperation,
                    [&](std::unique_ptr<LiveImageOp> &iop) { 
                      #if 0
                      PixelOp *pop = dynamic_cast<PixelOp *>(iop);
                      if (pop) {
                        //p->postAccum(this, tile);
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
      DFB_writeTile((ispc::VaryingTile*)&tile->finished, &tile->color);
    }

    auto msg = [&]{
      MasterTileMessageBuilder msg(colorBufferFormat, hasDepthBuffer,
                                   hasNormalBuffer, hasAlbedoBuffer,
                                   tile->begin, tile->error);
      msg.setColor(tile->color);
      msg.setDepth(tile->finished.z);
      msg.setNormal((vec3f*)tile->finished.nx);
      msg.setAlbedo((vec3f*)tile->finished.ar);
      return msg;
    };

    // TODO still send normal & albedo
    if (colorBufferFormat == OSP_FB_NONE) {
      std::lock_guard<std::mutex> lock(tileErrorsMutex);
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

  TileDesc* DFB::getTileDescFor(const vec2i &coords) const
  {
    return allTiles[getTileIDof(coords)].get();
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
      std::lock_guard<std::mutex> lock(mutex);
      if (!frameIsActive) {
        // TODO will: probably remove, but test this for now.
        PING;
        throw std::runtime_error("Somehow recieved a tile message when frame inactive!?");
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
    std::vector<int> tileBytesExpected(workerSize(), 0);
    std::vector<int> processOffsets(workerSize(), 0);
    if (IamTheMaster()) {
      tileGatherResult.resize(totalTilesExpected * tileSize);
      for (size_t i = 0; i < numTilesExpected.size(); ++i) {
        tileBytesExpected[i] = numTilesExpected[i] * tileSize;
      }
      size_t recvOffset = 0;
      for (int i = 0; i < workerSize(); ++i) {
#if 0
        std::cout << "Expecting " << tileBytesExpected[i] << " bytes from " << i
          << ", #tiles: " << numTilesExpected[i] << "\n" << std::flush;
#endif
        processOffsets[i] = recvOffset;
        recvOffset += tileBytesExpected[i];
      }
    }

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
    std::vector<int> gatherSizes(workerSize(), 0);
    gather(&sendCompressedSize, 1, MPI_INT,
           gatherSizes.data(), 1, MPI_INT,
           masterRank(), mpiGroup.comm).wait();

    std::vector<int> compressedOffsets(workerSize(), 0);
    int offset = 0;
    for (size_t i = 0; i < gatherSizes.size(); ++i) {
      compressedOffsets[i] = offset;
      offset += gatherSizes[i];
    }

    compressedResults.resize(offset, 0);
    gatherv(compressedBuf.data(), sendCompressedSize, MPI_BYTE,
            compressedResults.data(), gatherSizes, compressedOffsets,
            MPI_BYTE, masterRank(), mpiGroup.comm).wait();

    auto endGather = high_resolution_clock::now();

    if (IamTheMaster()) {
      // Now we must decompress each ranks data to process it, though we
      // already know how much data each is sending us and where to write it.
      auto startCompr = high_resolution_clock::now();
      tasking::parallel_for(workerSize(), [&](int i) {
          snappy::RawUncompress(&compressedResults[compressedOffsets[i]],
                                gatherSizes[i],
                                &tileGatherResult[processOffsets[i]]);
      });
      auto endCompr = high_resolution_clock::now();
      decompressTime = duration_cast<RealMilliseconds>(endCompr - startCompr);
    }
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

    std::vector<int> tilesFromRank(workerSize(), 0);
    const int myTileCount = tileIDs.size();
    gather(&myTileCount, 1, MPI_INT,
           tilesFromRank.data(), 1, MPI_INT,
           masterRank(), mpiGroup.comm).wait();

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
    std::memcpy(sendBuffer.data(), tileIDs.data(), tileIDs.size() * sizeof(vec2i));
    std::memcpy(sendBuffer.data() + tileIDs.size() * sizeof(vec2i),
                tileErrors.data(), tileErrors.size() * sizeof(float));

    gatherv(sendBuffer.data(), sendBuffer.size(), MPI_BYTE,
            tileGatherResult.data(), tileBytesExpected, processOffsets,
            MPI_BYTE, masterRank(), mpiGroup.comm).wait();

    if (IamTheMaster()) {
      tasking::parallel_for(workerSize(), [&](int rank) {
        const vec2i *tileID =
          reinterpret_cast<vec2i*>(tileGatherResult.data() + processOffsets[rank]);
        const float *error =
          reinterpret_cast<float*>(tileGatherResult.data() + processOffsets[rank]
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
      mpi::messaging::sendTo(dstRank, myId, msg);
    } else {
      if (!frameIsActive)
        throw std::runtime_error("#dfb: cannot setTile if frame is inactive!");

      LiveTileOperation *td = (LiveTileOperation*)tileDesc;
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
    if (localFBonMaster && !imageOps.empty() && firstFrameOperation < imageOps.size()) {
      std::for_each(imageOps.begin() + firstFrameOperation,
                    imageOps.end(),
                    [&](std::unique_ptr<LiveImageOp> &iop) {
                      LiveFrameOp *fop = dynamic_cast<LiveFrameOp *>(iop.get());
                      if (fop)
                        fop->process(camera);
                    });
    }

    std::for_each(imageOps.begin(), imageOps.end(),
                  [](std::unique_ptr<LiveImageOp> &p) { p->endFrame(); });

    // only refine on master
    if (mpicommon::IamTheMaster()) {
      frameVariance = tileErrorRegion.refine(errorThreshold);
    }

    if (hasAccumBuffer) {
      std::transform(tileAccumID.begin(), tileAccumID.end(), tileAccumID.begin(),
                     [](const uint32_t &x) { return x + 1; });
    }

    setCompletedEvent(OSP_FRAME_FINISHED);
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
    auto maxReduce = mpicommon::reduce(&localWaitTime, &maxWaitTime, 1,
                                       MPI_DOUBLE, MPI_MAX, 0, mpiGroup.comm);
    auto minReduce = mpicommon::reduce(&localWaitTime, &minWaitTime, 1,
                                       MPI_DOUBLE, MPI_MIN, 0, mpiGroup.comm);
    maxReduce.wait();
    minReduce.wait();

    if (mpiGroup.rank == 0) {
      os << "Max gather time: " << maxWaitTime << "ms\n"
        << "Min gather time: " << minWaitTime << "ms\n"
        << "Master tile write loop time: " << masterTileWriteTime.count() << "ms\n"
        << "Decompress time: " << decompressTime.count() << "ms\n";
    }
#endif
  }

} // ::ospray

