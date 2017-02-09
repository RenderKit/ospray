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

#ifdef _WIN32
#  include <windows.h> // for Sleep
#endif

#define DBG(a) /* ignore */

using std::cout;
using std::endl;

namespace ospray {

  // Helper types /////////////////////////////////////////////////////////////

  using DFB = DistributedFrameBuffer;

  struct MasterTileMessage : public mpi::async::CommLayer::Message {
    vec2i coords;
    float error;
  };

  /*! message sent to the master when a tile is finished. Todo:
      compress the color data */
  template <typename FBType>
  struct MasterTileMessage_FB : public MasterTileMessage {
    FBType color[TILE_SIZE][TILE_SIZE];
  };

  using MasterTileMessage_RGBA_I8  = MasterTileMessage_FB<uint32>;
  using MasterTileMessage_RGBA_F32 = MasterTileMessage_FB<vec4f>;
  using MasterTileMessage_NONE     = MasterTileMessage;

  /*! message sent from one node's instance to another, to tell that
      instance to write that tile */
  struct WriteTileMessage : public mpi::async::CommLayer::Message {
    // TODO: add compression of pixels during transmission
    vec2i coords; // XXX redundant: it's also in tile.region.lower
    ospray::Tile tile;
  };

  /*! color buffer and depth buffer on master */

  enum COMMANDTAG {
    /*! command tag that identifies a CommLayer::message as a write
      tile command. this is a command using for sending a tile of
      new samples to another instance of the framebuffer (the one
      that actually owns that tile) for processing and 'writing' of
      that tile on that owner node. */
    WORKER_WRITE_TILE = 13,
    /*! command tag used for sending 'final' tiles from the tile
        owner to the master frame buffer. Note that we *do* send a
        message back ot the master even in cases where the master
        does not actually care about the pixel data - we still have
        to let the master know when we're done. */
    MASTER_WRITE_TILE_I8,
    MASTER_WRITE_TILE_F32,
    /*! command tag used for sending 'final' tiles from the tile
        owner to the master frame buffer. Note that we *do* send a
        message back ot the master even in cases where the master
        does not actually care about the pixel data - we still have
        to let the master know when we're done. */
    MASTER_WRITE_TILE_NONE,
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

    int rc = MPI_Bcast(tileErrorBuffer, tiles, MPI_FLOAT, 0, MPI_COMM_WORLD);
    mpi::checkMpiError(rc);
  }

  // DistributedFrameBuffer definitions ///////////////////////////////////////

  DFB::DistributedFrameBuffer(mpi::async::CommLayer *comm,
                              const vec2i &numPixels,
                              size_t myID,
                              ColorBufferFormat colorBufferFormat,
                              bool hasDepthBuffer,
                              bool hasAccumBuffer,
                              bool hasVarianceBuffer)
    : FrameBuffer(numPixels,colorBufferFormat,hasDepthBuffer,
                  hasAccumBuffer,hasVarianceBuffer),
      mpi::async::CommLayer::Object(comm,myID),
      tileErrorRegion(hasVarianceBuffer ? getNumTiles() : vec2i(0)),
      localFBonMaster(nullptr),
      frameMode(WRITE_ONCE),
      frameIsActive(false),
      frameIsDone(false)
  {
    assert(comm);
    this->ispcEquivalent = ispc::DFB_create(this);
    ispc::DFB_set(getIE(), numPixels.x, numPixels.y, colorBufferFormat);
    comm->registerObject(this,myID);

    createTiles();
    const size_t bytes = sizeof(int32)*getTotalTiles();
    tileAccumID = (int32*)alignedMalloc(bytes);
    memset(tileAccumID, 0, bytes);

    if (comm->group->rank == 0) {
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
  }

  void DFB::startNewFrame(const float errorThreshold)
  {
    std::vector<mpi::async::CommLayer::Message *> delayedMessage;
    {
      SCOPED_LOCK(mutex);
      DBG(printf("rank %i starting new frame\n",mpi::world.rank));
      assert(!frameIsActive);

      if (pixelOp)
        pixelOp->beginFrame();

      for (auto &tile : myTiles)
        tile->newFrame();

      // create a local copy of delayed tiles, so we can work on them outside
      // the mutex
      delayedMessage = this->delayedMessage;
      this->delayedMessage.clear();

      tileErrorRegion.sync();
      numTilesCompletedThisFrame = 0;

      if (hasAccumBuffer) {
        // increment accumID only for active tiles
        for (int t = 0; t < getTotalTiles(); t++)
          if (tileError(vec2i(t, 0)) <= errorThreshold) {
            if (IamTheMaster() || allTiles[t]->mine())
              numTilesCompletedThisFrame++;
          } else
            tileAccumID[t]++;
      }

      frameIsDone   = false;

      // set frame to active - this HAS TO BE the last thing we do
      // before unlockign the mutex, because the 'incoming()' message
      // will actually NOT lock the mutex when checking if
      // 'frameIsActive' is true: as soon as the frame is tagged active,
      // incoming WILL write into the frame buffer, composite tiles,
      // etc!
      frameIsActive = true;
    }

    // might actually want to move this to a thread:
    for (auto &msg : delayedMessage)
      this->incoming(msg);

    if (numTilesCompletedThisFrame
        == (IamTheMaster() ? getTotalTiles() : myTiles.size()))
      closeCurrentFrame();
  }

  void DFB::freeTiles()
  {
    for (auto &tile : allTiles)
      delete tile;

    allTiles.clear();
    myTiles.clear();
  }

  TileData *DFB::createTile(const vec2i &xy, size_t tileID, size_t ownerID)
  {
    TileData *td = nullptr;

    switch(frameMode) {
    case WRITE_ONCE:
      td = new WriteOnlyOnceTile(this, xy, tileID, ownerID);
      break;
    case ALPHA_BLEND:
      td = new AlphaBlendTile_simple(this, xy, tileID, ownerID);
      break;
    case Z_COMPOSITE:
    default:
      td = new ZCompositeTile(this, xy, tileID, ownerID);
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
        size_t ownerID = tileID % (comm->group->size - 1);
        if (workerRank(ownerID) == comm->group->rank) {
          TileData *td = createTile(vec2i(x, y), tileID, ownerID);
          myTiles.push_back(td);
          allTiles.push_back(td);
        } else {
          allTiles.push_back(new TileDesc(this, vec2i(x,y), tileID, ownerID));
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
      if ((accumID(tileID) & 1) == 1)
        tileErrorRegion.update(tileID, msg->error);
    }

    // and finally, tell the master that this tile is done
    auto *tileDesc = this->getTileDescFor(msg->coords);
    TileData *td = (TileData*)tileDesc;
    this->tileIsCompleted(td);
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
    DBG(printf("rank %i: tilecompleted %i,%i\n",mpi::world.rank,
               tile->begin.x,tile->begin.y));
    if (IamTheMaster()) {
      int numTilesCompletedByMyTile = 0;
      /*! we will not do anything with the tile other than mark it's done */
      {
        SCOPED_LOCK(mutex);
        numTilesCompletedByMyTile = ++numTilesCompletedThisFrame;
        DBG(printf("MASTER: MARKING AS COMPLETED %i,%i -> %li %i\n",
                   tile->begin.x,tile->begin.y,numTilesCompletedThisFrame,
                   numTiles.x*numTiles.y));
      }
      if (numTilesCompletedByMyTile == numTiles.x*numTiles.y)
        closeCurrentFrame();
    } else {
      if (pixelOp) {
        pixelOp->postAccum(tile->final);
      }

      switch(colorBufferFormat) {
      case OSP_FB_NONE: {
        /* if the master doesn't have a framebufer (i.e., format
           'none'), we're only telling it that we're done, but are not
           sending any pixels */
        MasterTileMessage_NONE *mtm = new MasterTileMessage_NONE;
        mtm->command = MASTER_WRITE_TILE_NONE;
        mtm->coords  = tile->begin;
        mtm->error   = tile->error;
        comm->sendTo(this->master,mtm,sizeof(*mtm));
      } break;
      case OSP_FB_RGBA8:
      case OSP_FB_SRGBA: {
        /*! if the master has RGBA8 or SRGBA format, we're sending him a tile
          of the proper data */
        MasterTileMessage_RGBA_I8 *mtm = new MasterTileMessage_RGBA_I8;
        mtm->command = MASTER_WRITE_TILE_I8;
        mtm->coords  = tile->begin;
        mtm->error   = tile->error;
        memcpy(mtm->color,tile->color,TILE_SIZE*TILE_SIZE*sizeof(uint32));
        comm->sendTo(this->master,mtm,sizeof(*mtm));
      } break;
      case OSP_FB_RGBA32F: {
        /*! if the master has RGBA32F format, we're sending him a tile of the
          proper data */
        MasterTileMessage_RGBA_F32 *mtm = new MasterTileMessage_RGBA_F32;
        mtm->command = MASTER_WRITE_TILE_F32;
        mtm->coords  = tile->begin;
        mtm->error   = tile->error;
        memcpy(mtm->color,tile->color,TILE_SIZE*TILE_SIZE*sizeof(vec4f));
        comm->sendTo(this->master,mtm,sizeof(*mtm));
      } break;
      default:
        throw std::runtime_error("#osp:mpi:dfb: color buffer format not "
                                 "implemented for distributed frame buffer");
      };

      size_t numTilesCompletedByMe = 0;
      {
        SCOPED_LOCK(mutex);
        numTilesCompletedByMe = ++numTilesCompletedThisFrame;
        DBG(printf("rank %i: MARKING AS COMPLETED %i,%i -> %i %i\n",
                   mpi::world.rank,
                   tile->begin.x,tile->begin.y,(int)numTilesCompletedThisFrame,
                   numTiles.x*numTiles.y));
      }

      if (numTilesCompletedByMe == myTiles.size()) {
        closeCurrentFrame();
      }
    }
  }

  void DFB::incoming(mpi::async::CommLayer::Message *_msg)
  {
    if (!frameIsActive) {
      SCOPED_LOCK(mutex);
      if (!frameIsActive) {
        // frame is not actually active, yet - put the tile into the
        // delayed processing buffer, and return WITHOUT deleting it.
        delayedMessage.push_back(_msg);
        return;
      }
    }

    schedule([=]() {
      switch (_msg->command) {
      case MASTER_WRITE_TILE_NONE:
        this->processMessage((MasterTileMessage_NONE*)_msg);
        break;
      case MASTER_WRITE_TILE_I8:
        this->processMessage((MasterTileMessage_RGBA_I8*)_msg);
        break;
      case MASTER_WRITE_TILE_F32:
        this->processMessage((MasterTileMessage_RGBA_F32*)_msg);
        break;
      case WORKER_WRITE_TILE:
        this->processMessage((WriteTileMessage*)_msg);
        break;
      default:
        assert(0);
      };
      delete _msg;
    });
  }

  void DFB::closeCurrentFrame()
  {
    DBG(printf("rank %i CLOSES frame\n",mpi::world.rank));
    frameIsActive = false;
    frameIsDone   = true;

    if (IamTheMaster()) {
      /* do nothing */
    } else {
      if (pixelOp) { 
        pixelOp->endFrame();
      }
    }

    frameDoneCond.notify_all();
  }

  //! write given tile data into the frame buffer, sending to remote owner if
  //! required
  void DFB::setTile(ospray::Tile &tile)
  {
    auto *tileDesc = this->getTileDescFor(tile.region.lower);

    if (!tileDesc->mine()) {
      // NOT my tile...
      WriteTileMessage *msg = new WriteTileMessage;
      msg->coords = tile.region.lower;
      // TODO: compress pixels before sending ...
      memcpy(&msg->tile,&tile,sizeof(ospray::Tile));
      msg->command = WORKER_WRITE_TILE;

      comm->sendTo(this->worker[tileDesc->ownerID], msg,sizeof(*msg));
    } else {
      // this is my tile...
      assert(frameIsActive);

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
      parallel_for(myTiles.size(), [&](int taskIndex){
        TileData *td = this->myTiles[taskIndex];
        assert(td);
        if (fbChannelFlags & OSP_FB_ACCUM) {
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->accum.r[i] = 0.f;
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->accum.g[i] = 0.f;
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->accum.b[i] = 0.f;
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->accum.a[i] = 0.f;
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->accum.z[i] = inf;
        }
        if (fbChannelFlags & OSP_FB_DEPTH)
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->final.z[i] = inf;
        if (fbChannelFlags & OSP_FB_COLOR) {
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->final.r[i] = 0.f;
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->final.g[i] = 0.f;
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->final.b[i] = 0.f;
          for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) td->final.a[i] = 0.f;
        }
      });
    }

    if (hasAccumBuffer && (fbChannelFlags & OSP_FB_ACCUM)) {
      // we increment at the start of the frame
      memset(tileAccumID, -1, getTotalTiles()*sizeof(int32));
      tileErrorRegion.clear();
    }
  }

  int32 DFB::accumID(const vec2i &tile)
  {
    return tileAccumID[tile.y * numTiles.x + tile.x];
  }

  float DFB::tileError(const vec2i &tile)
  {
    return tileErrorRegion[tile];
  }

  float DFB::endFrame(const float errorThreshold)
  {
    return tileErrorRegion.refine(errorThreshold);
  }

} // ::ospray
