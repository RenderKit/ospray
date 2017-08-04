// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

  /*! message sent to the master when a tile is finished. TODO:
      compress the color data */
  template <typename FBType>
  struct MasterTileMessage_FB : public MasterTileMessage
  {
    FBType color[TILE_SIZE * TILE_SIZE];
  };

  template <typename ColorT>
  struct MasterTileMessage_FB_Depth : public MasterTileMessage_FB<ColorT>
  {
    float depth[TILE_SIZE * TILE_SIZE];
  };
  // TODO: Maybe we want to add a separate MasterTileMessage_FBD
  // that has an additional depth channel it ships along? If we
  // only care about colors I don't want to force the added
  // 4 * TILE_SIZE * TILE_SIZE bytes on the messaging system
  // since it will add quite a bit of overhead.

  using MasterTileMessage_RGBA_I8    = MasterTileMessage_FB<uint32>;
  using MasterTileMessage_RGBA_F32   = MasterTileMessage_FB<vec4f>;
  using MasterTileMessage_RGBA_I8_Z  = MasterTileMessage_FB_Depth<uint32>;
  using MasterTileMessage_RGBA_F32   = MasterTileMessage_FB<vec4f>;
  using MasterTileMessage_RGBA_F32_Z = MasterTileMessage_FB_Depth<vec4f>;
  using MasterTileMessage_NONE       = MasterTileMessage;

  /*! It's a real PITA do try and do this using the message structs
   * but keep them POD and also try to not copy a ton.
   */
  class MasterTileMessageBuilder
  {
    OSPFrameBufferFormat colorFormat;
    bool hasDepth;
    size_t pixelSize;
    MasterTileMessage_NONE *header;

  public:
    std::shared_ptr<mpicommon::Message> message;

    MasterTileMessageBuilder(OSPFrameBufferFormat fmt, bool hasDepth,
        vec2i coords, float error)
      : colorFormat(fmt), hasDepth(hasDepth)
    {
      int command;
      size_t msgSize = 0;
      switch (colorFormat) {
        case OSP_FB_NONE:
          command = MASTER_WRITE_TILE_NONE;
          msgSize = sizeof(MasterTileMessage_NONE);
          pixelSize = 0;
          break;
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
        default:
          throw std::runtime_error("Unsupported color buffer fmt in DFB!");
      }
      if (hasDepth) {
        msgSize += sizeof(float) * TILE_SIZE * TILE_SIZE;
        command = command | MASTER_TILE_HAS_DEPTH;
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

    MPI_CALL(Bcast(tileErrorBuffer, tiles, MPI_FLOAT, 0, MPI_COMM_WORLD));
  }

  // DistributedFrameBuffer definitions ///////////////////////////////////////

  DFB::DistributedFrameBuffer(const vec2i &numPixels,
                              ObjectHandle myID,
                              ColorBufferFormat colorBufferFormat,
                              bool hasDepthBuffer,
                              bool hasAccumBuffer,
                              bool hasVarianceBuffer,
                              bool masterIsAWorker)
    : FrameBuffer(numPixels,colorBufferFormat,hasDepthBuffer,
                  hasAccumBuffer,hasVarianceBuffer),
      myID(myID),
      tileErrorRegion(hasVarianceBuffer ? getNumTiles() : vec2i(0)),
      localFBonMaster(nullptr),
      frameMode(WRITE_MULTIPLE),
      frameIsActive(false),
      frameIsDone(false),
      masterIsAWorker(masterIsAWorker)
  {
    this->ispcEquivalent = ispc::DFB_create(this);
    ispc::DFB_set(getIE(), numPixels.x, numPixels.y, colorBufferFormat);

    mpi::messaging::registerMessageListener(myID.objID(), this);

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
        localFBonMaster = new LocalFrameBuffer(numPixels,
                                               colorBufferFormat,
                                               hasDepthBuffer,
                                               false,
                                               false);
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
    std::vector<std::shared_ptr<mpicommon::Message>> delayedMessage;

    {
      SCOPED_LOCK(mutex);
      DBG(printf("rank %i starting new frame\n", mpicommon::globalRank()));
      assert(!frameIsActive);

      if (pixelOp)
        pixelOp->beginFrame();

      // create a local copy of delayed tiles, so we can work on them outside
      // the mutex
      delayedMessage = this->delayedMessage;
      this->delayedMessage.clear();

      // NOTE: Doing error sync may do a broadcast, needs to be done before
      //       async messaging enabled in beginFrame()
      tileErrorRegion.sync();
      MPI_CALL(Bcast(tileInstances, getTotalTiles(), MPI_INT, 0, MPI_COMM_WORLD));

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
    for (auto &msg : delayedMessage)
      this->incoming(msg);

    if (isFrameComplete())
      closeCurrentFrame();
  }

  void DFB::freeTiles()
  {
    for (auto &tile : allTiles)
      delete tile;

    allTiles.clear();
    myTiles.clear();
  }

  bool DFB::isFrameComplete()
  {
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
    default:
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

  const void *DFB::mapDepthBuffer()
  {
    if (!localFBonMaster) {
      throw std::runtime_error("#osp:mpi:dfb: tried to 'ospMap()' the depth "
                               "buffer of a frame buffer that doesn't have a "
                               "host-side color buffer");
    }
    assert(localFBonMaster);
    return localFBonMaster->mapDepthBuffer();
  }

  const void *DFB::mapColorBuffer()
  {
    if (!localFBonMaster) {
      throw std::runtime_error("#osp:mpi:dfb: tried to 'ospMap()' the color "
                               "buffer of a frame buffer that doesn't have a "
                               "host-side color buffer");
    }
    assert(localFBonMaster);
    return localFBonMaster->mapColorBuffer();
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

  void DFB::processMessage(MasterTileMessage *msg)
  {
    /* just update error for 'none' tiles */
    if (hasVarianceBuffer) {
      const vec2i tileID = msg->coords/TILE_SIZE;
      if (msg->error < (float)inf)
        tileErrorRegion.update(tileID, msg->error);
    }

    // and finally, tell the master that this tile is done
    auto *tileDesc = this->getTileDescFor(msg->coords);
    TileData *td = (TileData*)tileDesc;
    this->finalizeTileOnMaster(td);
  }

  void DFB::processMessage(WriteTileMessage *msg)
  {
    auto *tileDesc = this->getTileDescFor(msg->coords);
    // TODO: compress/decompress tile data
    TileData *td = (TileData*)tileDesc;
    td->process(msg->tile);
  }

  void DFB::tileIsCompleted(TileData *tile)
  {
    DBG(printf("rank %i: tilecompleted %i,%i\n",mpicommon::globalRank(),
               tile->begin.x,tile->begin.y));

    if (pixelOp) {
      pixelOp->postAccum(tile->final);
    }

    MasterTileMessageBuilder msg(colorBufferFormat, hasDepthBuffer,
                                 tile->begin, tile->error);
    msg.setColor(tile->color);
    msg.setDepth(tile->final.z);

    // Note: In the data-distributed device the master will be rendering
    // and completing tiles.
    if (mpicommon::IamAWorker()) {
      mpi::messaging::sendTo(mpicommon::masterRank(), myID, msg.message);
      numTilesCompletedThisFrame++;

      DBG(printf("RANK %d MARKING AS COMPLETED %i,%i -> %i/%i\n",
                 mpicommon::globalRank(), tile->begin.x, tile->begin.y,
                 numTilesCompletedThisFrame.load(), myTiles.size()));

      if (isFrameComplete()) {
        closeCurrentFrame();
      }
    } else {
      // If we're the master sending a message to ourself skip going
      // through the messaging layer entirely and just call incoming directly
      incoming(msg.message);
    }
  }

  void DFB::finalizeTileOnMaster(TileData *tile)
  {
    assert(mpicommon::IamTheMaster());
    /*! we will not do anything with the tile other than mark it's done */
    numTilesCompletedThisFrame++;

    DBG(printf("MASTER MARKING AS COMPLETED %i,%i -> %i/%i\n",
               tile->begin.x, tile->begin.y,
               numTilesCompletedThisFrame.load(), getTotalTiles()));

    if (isFrameComplete()) {
      closeCurrentFrame();
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
    if (!frameIsActive) {
      SCOPED_LOCK(mutex);
      if (!frameIsActive) {
        // frame is not actually active, yet - put the tile into the
        // delayed processing buffer, and return WITHOUT deleting it.
        delayedMessage.push_back(message);
        return;
      }
    }

    tasking::schedule([=]() {
      auto *_msg = (TileMessage*)message->data;
      if (_msg->command & MASTER_WRITE_TILE_NONE) {
        this->processMessage((MasterTileMessage_NONE*)_msg);
      } else if (_msg->command & MASTER_WRITE_TILE_I8) {
        this->processMessage((MasterTileMessage_RGBA_I8*)_msg);
      } else if (_msg->command & MASTER_WRITE_TILE_F32) {
        this->processMessage((MasterTileMessage_RGBA_F32*)_msg);
      } else if (_msg->command & WORKER_WRITE_TILE) {
        this->processMessage((WriteTileMessage*)_msg);
      } else {
        throw std::runtime_error("#dfb: unknown tile type processed!");
      }
    });
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
      mpi::messaging::sendTo(dstRank, myID, msg);
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
      tasking::parallel_for(myTiles.size(), [&](int taskIndex) {
        TileData *td = this->myTiles[taskIndex];
        assert(td);
        const auto bytes = TILE_SIZE * TILE_SIZE * sizeof(float);
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
