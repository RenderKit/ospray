// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "DistributedFrameBuffer.h"
#include "DistributedFrameBuffer_TileTypes.h"
#include "DistributedFrameBuffer_ispc.h"

#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/tasking/schedule.h"

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

namespace ospray {

  // Helper types /////////////////////////////////////////////////////////////

  using DFB = DistributedFrameBuffer;

  struct TileMessage
  {
    int command {-1};
  };

  struct MasterTileMessage : public TileMessage
  {
    vec2i coords;
    float error;
  };

  struct AllTilesDoneMessage : public TileMessage
  {
      int numTiles {0};

      vec2i* tileIDs(ospcommon::byte_t* data)
      {
          return (vec2i*)(data + sizeof(TileMessage) + sizeof(int));
      }

      float* tileErrors(ospcommon::byte_t* data)
      {
          return (float*)(data + sizeof(TileMessage) + sizeof(int) + numTiles * sizeof(vec2i));
      }

      static size_t size(const size_t numTiles)
      {
          return sizeof(TileMessage) + sizeof(int) + sizeof(int) +
                  numTiles * (sizeof(vec2i) + sizeof(float));
      }
  };

  /*! message sent to the master when a tile is finished. TODO:
      compress the color data */
  template <typename ColorT>
  struct MasterTileMessage_FB : public MasterTileMessage
  {
    ColorT color[TILE_SIZE * TILE_SIZE];
  };

  template <typename ColorT>
  struct MasterTileMessage_FB_Depth : public MasterTileMessage_FB<ColorT>
  {
    float depth[TILE_SIZE * TILE_SIZE];
  };

  template <typename ColorT>
  struct MasterTileMessage_FB_Depth_Aux : public MasterTileMessage_FB_Depth<ColorT>
  {
    vec3f normal[TILE_SIZE * TILE_SIZE];
    vec3f albedo[TILE_SIZE * TILE_SIZE];
  };

  using MasterTileMessage_RGBA_I8    = MasterTileMessage_FB<uint32>;
  using MasterTileMessage_RGBA_I8_Z  = MasterTileMessage_FB_Depth<uint32>;
  using MasterTileMessage_RGBA8_Z_AUX = MasterTileMessage_FB_Depth_Aux<uint32>;
  using MasterTileMessage_RGBA_F32   = MasterTileMessage_FB<vec4f>;
  using MasterTileMessage_RGBA_F32_Z = MasterTileMessage_FB_Depth<vec4f>;
  using MasterTileMessage_RGBAF32_Z_AUX = MasterTileMessage_FB_Depth_Aux<vec4f>;
  using MasterTileMessage_NONE       = MasterTileMessage;

  /*! It's a real PITA do try and do this using the message structs
   * but keep them POD and also try to not copy a ton.
   */
  class MasterTileMessageBuilder
  {
    OSPFrameBufferFormat colorFormat;
    bool hasDepth;
    bool hasNormal;
    bool hasAlbedo;
    size_t pixelSize;
    MasterTileMessage_NONE *header;

  public:
    std::shared_ptr<mpicommon::Message> message;

    MasterTileMessageBuilder(OSPFrameBufferFormat fmt, bool hasDepth,
        bool hasNormal, bool hasAlbedo,
        vec2i coords, float error)
      : colorFormat(fmt), hasDepth(hasDepth),
        hasNormal(hasNormal), hasAlbedo(hasAlbedo)
    {
      int command = 0;
      size_t msgSize = 0;
      switch (colorFormat) {
        case OSP_FB_NONE:
          throw std::runtime_error("Do not use per tile message for FB_NONE!");
        case OSP_FB_RGBA8:
        case OSP_FB_SRGBA:
          command = MASTER_WRITE_TILE_I8;
          msgSize = sizeof(MasterTileMessage_RGBA_I8);
          pixelSize = sizeof(uint32);
          break;
        case OSP_FB_RGBA32F:
          command = MASTER_WRITE_TILE_F32;
          msgSize = sizeof(MasterTileMessage_RGBA_F32);
          pixelSize = sizeof(vec4f);
          break;
      }
      // AUX also includes depth
      if (hasDepth || hasNormal || hasAlbedo) {
        msgSize += sizeof(float) * TILE_SIZE * TILE_SIZE;
        if (hasDepth)
          command |= MASTER_TILE_HAS_DEPTH;
      }
      if (hasNormal || hasAlbedo) {
        msgSize += 2 * sizeof(vec3f) * TILE_SIZE * TILE_SIZE;
        command |= MASTER_TILE_HAS_AUX;
      }
      message = std::make_shared<mpicommon::Message>(msgSize);
      header = reinterpret_cast<MasterTileMessage_NONE*>(message->data);
      header->command = command;
      header->coords = coords;
      header->error = error;
    }
    void setColor(const vec4f *color) {
      if (colorFormat != OSP_FB_NONE) {
        const uint8_t *input = reinterpret_cast<const uint8_t*>(color);
        uint8_t *out = message->data + sizeof(MasterTileMessage_NONE);
        std::copy(input, input + pixelSize * TILE_SIZE * TILE_SIZE, out);
      }
    }
    void setDepth(const float *depth) {
      if (hasDepth) {
        float *out = reinterpret_cast<float*>(message->data
                     + sizeof(MasterTileMessage_NONE)
                     + pixelSize * TILE_SIZE * TILE_SIZE);
        std::copy(depth, depth + TILE_SIZE * TILE_SIZE, out);
      }
    }
    void setNormal(const vec3f *normal) {
      if (hasNormal) {
        auto out = reinterpret_cast<vec3f*>(message->data
                     + sizeof(MasterTileMessage_NONE)
                     + pixelSize * TILE_SIZE * TILE_SIZE
                     + sizeof(float) * TILE_SIZE * TILE_SIZE);
        std::copy(normal, normal + TILE_SIZE * TILE_SIZE, out);
      }
    }
    void setAlbedo(const vec3f *albedo) {
      if (hasAlbedo) {
        auto out = reinterpret_cast<vec3f*>(message->data
                     + sizeof(MasterTileMessage_NONE)
                     + pixelSize * TILE_SIZE * TILE_SIZE
                     + sizeof(float) * TILE_SIZE * TILE_SIZE
                     + sizeof(vec3f) * TILE_SIZE * TILE_SIZE);
        std::copy(albedo, albedo + TILE_SIZE * TILE_SIZE, out);
      }
    }
  };

  /*! message sent from one node's instance to another, to tell that
      instance to write that tile */
  struct WriteTileMessage : public TileMessage
  {
    // TODO: add compression of pixels during transmission
    vec2i coords; // XXX redundant: it's also in tile.region.lower
    ospray::Tile tile;
  };

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
    std::vector<std::shared_ptr<mpicommon::Message>> _delayedMessage;

    {
      SCOPED_LOCK(mutex);
      DBG(printf("rank %i starting new frame\n", mpicommon::globalRank()));
      assert(!frameIsActive);

      if (pixelOp)
        pixelOp->beginFrame();

      // create a local copy of delayed tiles, so we can work on them outside
      // the mutex
      _delayedMessage = this->delayedMessage;
      this->delayedMessage.clear();

      // NOTE: Doing error sync may do a broadcast, needs to be done before
      //       async messaging enabled in beginFrame()
      tileErrorRegion.sync();
      MPI_CALL(Bcast(tileInstances, getTotalTiles(), MPI_INT, 0,
                     mpicommon::world.comm));

      if(colorBufferFormat == OSP_FB_NONE) {
        tileIDs.clear();
        tileErrors.clear();
        tileIDs.reserve(myTiles.size());
        tileErrors.reserve(myTiles.size());
      }

      // after Bcast of tileInstances (needed in WriteMultipleTile::newFrame)
      for (auto &tile : myTiles)
        tile->newFrame();

      numTilesCompletedThisFrame = 0;

      if (hasAccumBuffer) {
        for (int t = 0; t < getTotalTiles(); t++) {
          if (tileError(vec2i(t, 0)) <= errorThreshold) {
            if (mpicommon::IamTheMaster() || allTiles[t]->mine())
              numTilesCompletedThisFrame++;
          }
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

    // might actually want to move this to a thread:
    for (auto &msg : _delayedMessage)
      scheduleProcessing(msg);

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
    return numTilesCompletedThisFrame ==
      (mpicommon::IamAWorker() ? myTiles.size() : getTotalTiles());
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
          allTiles.push_back(new TileDesc(this, tileStart, tileID, ownerID));
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
    std::unique_lock<std::mutex> lock(mutex);
    frameDoneCond.wait(lock, [&]{return frameIsDone;});
  }

  void DFB::processMessage(AllTilesDoneMessage *msg, ospcommon::byte_t* data)
  {
    if (hasVarianceBuffer) {
      auto tileIDs = msg->tileIDs(data);
      auto tileErrors = msg->tileErrors(data);
      for (int i = 0; i < msg->numTiles; ++i) {
        if (tileErrors[i] < (float)inf)
            tileErrorRegion.update(tileIDs[i], tileErrors[i]);
      }
    }

    if (isFrameComplete(msg->numTiles)) {
      closeCurrentFrame();
    }
  }

  void DFB::processMessage(WriteTileMessage *msg)
  {
    auto *tileDesc = this->getTileDescFor(msg->coords);
    // TODO: compress/decompress tile data
    TileData *td = (TileData*)tileDesc;
    td->process(msg->tile);
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

    // Finally, tell the master that this tile is done
    auto *tileDesc = this->getTileDescFor(msg->coords);
    TileData *td = (TileData*)tileDesc;
    this->finalizeTileOnMaster(td);
  }

  void DFB::tileIsCompleted(TileData *tile)
  {
    DBG(printf("rank %i: tilecompleted %i,%i\n",mpicommon::globalRank(),
               tile->begin.x,tile->begin.y));

    if (pixelOp)
      pixelOp->postAccum(tile->final);

    // write the final colors into the color buffer
    // normalize and write final color, and compute error
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
      if(colorBufferFormat == OSP_FB_NONE) { // TODO still send normal&albedo
        SCOPED_LOCK(tileErrorsMutex);
        tileIDs.push_back(tile->begin/TILE_SIZE);
        tileErrors.push_back(tile->error);
      }
      else
        mpi::messaging::sendTo(mpicommon::masterRank(), myId, msg().message);

      if (isFrameComplete(1)) {
        if(colorBufferFormat == OSP_FB_NONE)
          sendAllTilesDoneMessage();
        closeCurrentFrame();
      }

      DBG(printf("RANK %d MARKING AS COMPLETED %i,%i -> %i/%i\n",
                 mpicommon::globalRank(), tile->begin.x, tile->begin.y,
                 numTilesCompletedThisFrame, myTiles.size()));
    } else {
      // If we're the master sending a message to ourself skip going
      // through the messaging layer entirely and just call incoming directly
      incoming(msg().message);
    }
  }

  void DFB::finalizeTileOnMaster(TileData *DBG(tile))
  {
    assert(mpicommon::IamTheMaster());

    // TODO pixel accurate progress (tiles can be much smaller than TILE_SIZE)
    float progress = (numTilesCompletedThisFrame+1)/(float)getTotalTiles();
    if (!api::currentDevice().reportProgress(progress))
      sendCancelRenderingMessage();

    if (isFrameComplete(1)) {
      closeCurrentFrame();
    }

    DBG(printf("MASTER MARKING AS COMPLETED %i,%i -> %i/%i\n",
               tile->begin.x, tile->begin.y,
               numTilesCompletedThisFrame, getTotalTiles()));
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
    if (!frameIsActive) {
      SCOPED_LOCK(mutex);
      if (!frameIsActive) {
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
    auto *msg = (TileMessage*)message->data;
    if (msg->command & CANCEL_RENDERING) {
      cancelRendering = true;
      return;
    }

      tasking::schedule([=]() {
        if (msg->command & MASTER_WRITE_TILE_I8) {
          this->processMessage((MasterTileMessage_RGBA_I8*)msg);
        } else if (msg->command & MASTER_WRITE_TILE_F32) {
          this->processMessage((MasterTileMessage_RGBA_F32*)msg);
        } else if (msg->command & WORKER_WRITE_TILE) {
          this->processMessage((WriteTileMessage*)msg);
        } else if (msg->command & WORKER_ALL_TILES_DONE) {
          this->processMessage((AllTilesDoneMessage*)msg, message->data);
        } else {
          throw std::runtime_error("#dfb: unknown tile type processed!");
        }
      });
  }

  void DFB::sendAllTilesDoneMessage()
  {
      auto msg = std::make_shared<mpicommon::Message>
              (AllTilesDoneMessage::size(tileErrors.size()));

      auto out = msg->data;
      int val = WORKER_ALL_TILES_DONE;
      memcpy(out, &val, sizeof(val));
      out += sizeof(val);

      int numTiles = tileErrors.size();
      memcpy(out, &numTiles, sizeof(numTiles));
      out += sizeof(numTiles);

      if(!tileIDs.empty())
      {
          memcpy(out, tileIDs.data(), tileIDs.size() * sizeof(vec2i));
          out += tileIDs.size() * sizeof(vec2i);

          memcpy(out, tileErrors.data(), tileErrors.size() * sizeof(float));
      }

      mpi::messaging::sendTo(mpicommon::masterRank(), myId, msg);
  }

  void DFB::sendCancelRenderingMessage()
  {
      auto msg = std::make_shared<mpicommon::Message>(sizeof(TileMessage));

      auto out = msg->data;
      int val = CANCEL_RENDERING;
      memcpy(out, &val, sizeof(val));

      // notify all; broadcast not possible, because messaging layer is active
      for (int rank = 0; rank < mpicommon::numGlobalRanks(); rank++)
        mpi::messaging::sendTo(rank, myId, msg);
  }

  void DFB::closeCurrentFrame()
  {
    DBG(printf("rank %i CLOSES frame\n", mpicommon::globalRank()));

    if (mpicommon::IamTheMaster() && !masterIsAWorker) {
      /* do nothing */
    } else {
      if (pixelOp) {
        pixelOp->endFrame();
      }
    }

    SCOPED_LOCK(mutex);
    frameIsActive = false;
    frameIsDone   = true;
    frameDoneCond.notify_all();
  }

  //! write given tile data into the frame buffer, sending to remote owner if
  //! required
  void DFB::setTile(ospray::Tile &tile)
  {
    auto *tileDesc = this->getTileDescFor(tile.region.lower);

    if (!tileDesc->mine()) {
      // NOT my tile...
      WriteTileMessage msgPayload;
      msgPayload.coords = tile.region.lower;
      // TODO: compress pixels before sending ...
      memcpy(&msgPayload.tile, &tile, sizeof(ospray::Tile));
      msgPayload.command = WORKER_WRITE_TILE;

      auto msg = std::make_shared<mpicommon::Message>(&msgPayload,
                                                      sizeof(msgPayload));

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
    mpi::messaging::enableAsyncMessaging();
    FrameBuffer::beginFrame();
  }

  float DFB::endFrame(const float errorThreshold)
  {
    mpi::messaging::disableAsyncMessaging();
    memset(tileInstances, 0, sizeof(int32)*getTotalTiles()); // XXX needed?
    if (mpicommon::IamTheMaster()) // only refine on master
      return tileErrorRegion.refine(errorThreshold);
    else // slaves will get updated error with next sync() anyway
      return inf;
  }

} // ::ospray
