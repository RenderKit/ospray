// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "DistributedFrameBuffer_ispc.h"

#include "ospray/common/tasking/async.h"
#include "ospray/common/tasking/parallel_for.h"

#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h> // for Sleep
#endif

#define DBG(a) /* ignore */

#define USE_ALPHABLEND_TILE_TYPE 1 // NOTE(jda) - else ZComposite...

namespace ospray {
  using std::cout;
  using std::endl;

  inline int clientRank(int clientID) { return clientID+1; }

  using DFB = DistributedFrameBuffer;

  DFB::TileDesc::TileDesc(DFB *dfb,
                          const vec2i &begin,
                          size_t tileID,
                          size_t ownerID)
    : tileID(tileID), ownerID(ownerID), dfb(dfb), begin(begin)
  {}

  DFB::TileData::TileData(DFB *dfb,
                          const vec2i &begin,
                          size_t tileID,
                          size_t ownerID)
    : TileDesc(dfb,begin,tileID,ownerID)
  {}


  /*! called exactly once at the beginning of each frame */
  void DFB::AlphaBlendTile_simple::newFrame()
  {
    currentGeneration = 0;
    expectedInNextGeneration = 0;
    missingInCurrentGeneration = 1;

    assert(bufferedTile.empty());
  }

  void computeSortOrder(DFB::AlphaBlendTile_simple::BufferedTile *t)
  {
    float z = std::numeric_limits<float>::infinity();
    for (int iy=0;iy<t->tile.region.upper.y-t->tile.region.lower.y;iy++)
      for (int ix=0;ix<t->tile.region.upper.x-t->tile.region.lower.x;ix++)
        z = std::min(z,t->tile.z[ix+TILE_SIZE*iy]);
    t->sortOrder = z;
  }

  void DFB::TileData::accumulate(const ospray::Tile &tile)
  {
    vec2i dia = tile.region.upper - tile.region.lower;
    float pixelsf = (float)dia.x * dia.y;
    switch(dfb->colorBufferFormat) {
      case OSP_FB_RGBA8:
        error = ispc::DFB_accumulate_RGBA8(dfb->ispcEquivalent,
            (ispc::VaryingTile*)&tile,
            (ispc::VaryingTile*)&this->final,
            (ispc::VaryingTile*)&this->accum,
            (ispc::VaryingTile*)&this->variance,
            &this->color,
            pixelsf,
            dfb->accumId,
            dfb->hasAccumBuffer,
            dfb->hasVarianceBuffer);
        break;
      case OSP_FB_SRGBA:
        error = ispc::DFB_accumulate_SRGBA(dfb->ispcEquivalent,
            (ispc::VaryingTile*)&tile,
            (ispc::VaryingTile*)&this->final,
            (ispc::VaryingTile*)&this->accum,
            (ispc::VaryingTile*)&this->variance,
            &this->color,
            pixelsf,
            dfb->accumId,
            dfb->hasAccumBuffer,
            dfb->hasVarianceBuffer);
        break;
      case OSP_FB_RGBA32F:
        error = ispc::DFB_accumulate_RGBA32F(dfb->ispcEquivalent,
            (ispc::VaryingTile*)&tile,
            (ispc::VaryingTile*)&this->final,
            (ispc::VaryingTile*)&this->accum,
            (ispc::VaryingTile*)&this->variance,
            &this->color,
            pixelsf,
            dfb->accumId,
            dfb->hasAccumBuffer,
            dfb->hasVarianceBuffer);
        break;
    default:
      break;
    }
  }

  /*! called exactly once for each ospray::Tile that needs to get
    written into / composited into this dfb tile */
  void DFB::AlphaBlendTile_simple::process(const ospray::Tile &tile)
  {
    BufferedTile *addTile = new BufferedTile;
    memcpy(&addTile->tile,&tile,sizeof(tile));
    computeSortOrder(addTile);

    this->final.region = tile.region;
    this->final.fbSize = tile.fbSize;
    this->final.rcp_fbSize = tile.rcp_fbSize;

    {
      SCOPED_LOCK(mutex);
      bufferedTile.push_back(addTile);

      if (tile.generation == currentGeneration) {
        --missingInCurrentGeneration;
        expectedInNextGeneration += tile.children;
        while (missingInCurrentGeneration == 0 &&
               expectedInNextGeneration > 0) {
          currentGeneration++;
          missingInCurrentGeneration = expectedInNextGeneration;
          expectedInNextGeneration = 0;
          for (int i=0;i<bufferedTile.size();i++) {
            BufferedTile *bt = bufferedTile[i];
            if (bt->tile.generation == currentGeneration) {
              --missingInCurrentGeneration;
              expectedInNextGeneration += bt->tile.children;
            }
          }
        }
      }

      if (missingInCurrentGeneration == 0) {
        Tile **tileArray = STACK_BUFFER(Tile*, bufferedTile.size());
        for (int i = 0; i < bufferedTile.size(); i++) {
          tileArray[i] = &bufferedTile[i]->tile;
        }

        ispc::DFB_sortAndBlendFragments((ispc::VaryingTile **)tileArray,
                                        bufferedTile.size());

        this->final.region = tile.region;
        this->final.fbSize = tile.fbSize;
        this->final.rcp_fbSize = tile.rcp_fbSize;
        accumulate(bufferedTile[0]->tile);
        dfb->tileIsCompleted(this);
        for (auto &tile : bufferedTile)
          delete tile;
        bufferedTile.clear();
      }
    }
  }

  /*! called exactly once at the beginning of each frame */
  void DFB::WriteOnlyOnceTile::newFrame()
  {
    /* nothing to do for write-once tile - we *know* it'll get written
       only once */
  }

  /*! called exactly once for each ospray::Tile that needs to get
    written into / composited into this dfb tile.

    for a write-once tile, we expect this to be called exactly once
    per tile, so there's not a lot to do in here than accumulating the
    tile data and telling the parent that we're done.
  */
  void DFB::WriteOnlyOnceTile::process(const ospray::Tile &tile)
  {
    this->final.region = tile.region;
    this->final.fbSize = tile.fbSize;
    this->final.rcp_fbSize = tile.rcp_fbSize;
    accumulate(tile);
    dfb->tileIsCompleted(this);
  }

  void DFB::ZCompositeTile::newFrame()
  {
    numPartsComposited = 0;
  }

  void DFB::ZCompositeTile::process(const ospray::Tile &tile)
  {
    bool done = false;

    {
      SCOPED_LOCK(mutex);
      if (numPartsComposited == 0)
        memcpy(&compositedTileData,&tile,sizeof(tile));
      else
        ispc::DFB_zComposite((ispc::VaryingTile*)&tile,
                             (ispc::VaryingTile*)&this->compositedTileData);

      done = (++numPartsComposited == dfb->comm->numWorkers());
    }

    if (done) {
      accumulate(this->compositedTileData);
      dfb->tileIsCompleted(this);
    }
  }

  void DFB::startNewFrame()
  {
    std::vector<mpi::async::CommLayer::Message *> delayedMessage;
    {
      SCOPED_LOCK(mutex);
      DBG(printf("rank %i starting new frame\n",mpi::world.rank));
      assert(!frameIsActive);

      if (hasAccumBuffer)
        accumId++;

      if (pixelOp)
        pixelOp->beginFrame();

      for (auto &tile : myTiles)
        tile->newFrame();

      // create a local copy of delayed tiles, so we can work on them outside
      // the mutex
      delayedMessage = this->delayedMessage;
      this->delayedMessage.clear();
      numTilesCompletedThisFrame = 0;
      numTilesToMasterThisFrame = 0;

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
  }

  void DFB::freeTiles()
  {
    for (auto &tile : allTiles)
      delete tile;

    allTiles.clear();
    myTiles.clear();
  }

  DFB::TileData *DFB::createTile(const vec2i &xy, size_t tileID, size_t ownerID)
  {
    TileData *td = nullptr;

    if (frameMode == WRITE_ONCE)
      td = new WriteOnlyOnceTile(this, xy, tileID, ownerID);
    else {
#if USE_ALPHABLEND_TILE_TYPE
      td = new AlphaBlendTile_simple(this, xy, tileID, ownerID);
#else
      td = new ZCompositeTile(this, xy, tileID, ownerID);
#endif
    }

    return td;
  }

  void DFB::createTiles()
  {
    size_t tileID = 0;
    for (size_t y = 0; y < numPixels.y; y += TILE_SIZE) {
      for (size_t x = 0; x < numPixels.x; x += TILE_SIZE, tileID++) {
        size_t ownerID = tileID % (comm->group->size - 1);
        if (clientRank(ownerID) == comm->group->rank) {
          TileData *td = createTile(vec2i(x, y), tileID, ownerID);
          myTiles.push_back(td);
          allTiles.push_back(td);
        } else {
          allTiles.push_back(new TileDesc(this, vec2i(x,y), tileID, ownerID));
        }
      }
    }
  }

  DFB::DistributedFrameBuffer(mpi::async::CommLayer *comm,
                              const vec2i &numPixels,
                              size_t myID,
                              ColorBufferFormat colorBufferFormat,
                              bool hasDepthBuffer,
                              bool hasAccumBuffer,
                              bool hasVarianceBuffer)
    : mpi::async::CommLayer::Object(comm,myID),
      FrameBuffer(numPixels,colorBufferFormat,hasDepthBuffer,
                  hasAccumBuffer,hasVarianceBuffer),
      numPixels(numPixels),
      maxValidPixelID(numPixels-vec2i(1)),
      numTiles(divRoundUp(numPixels, getTileSize())),
      frameIsActive(false), frameIsDone(false), localFBonMaster(NULL),
      accumId(0),
      tileErrorBuffer(NULL),
      frameMode(WRITE_ONCE)
  {
    assert(comm);
    this->ispcEquivalent = ispc::DFB_create(this);
    ispc::DFB_set(getIE(),numPixels.x,numPixels.y, colorBufferFormat);
    comm->registerObject(this,myID);

    createTiles();

    if (comm->group->rank == 0) {
      if (colorBufferFormat == OSP_FB_NONE) {
        cout << "#osp:mpi:dfb: we're the master, but framebuffer has 'NONE' "
             << "format; creating distributed frame buffer WITHOUT having a "
             << "mappable copy on the master" << endl;
      } else {
        localFBonMaster = new LocalFrameBuffer(numPixels,
                                               colorBufferFormat,
                                               hasDepthBuffer,
                                               false,
                                               false);
      }
      if (hasVarianceBuffer) {
        tileErrorBuffer =
            (float*)alignedMalloc(sizeof(float)*numTiles.x*numTiles.y);
      }
    }

    ispc::DFB_set(getIE(),numPixels.x,numPixels.y,
                  colorBufferFormat);
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
    doneCond.wait(lock, [&]{return frameIsDone;});
  }

  void DFB::processMessage(DFB::WriteTileMessage *msg)
  {
    DFB::TileDesc *tileDesc = this->getTileDescFor(msg->coords);
    // TODO: compress/decompress tile data
    TileData *td = (TileData*)tileDesc;
    td->process(msg->tile);
  }

  void DFB::processMessage(MasterTileMessage_NONE *msg)
  {
    { /* nothing to do for 'none' tiles */ }
    if (hasVarianceBuffer && (accumId & 1) == 1)
      tileErrorBuffer[getTileIDof(msg->coords)] = msg->error;

    // and finally, tell the master that this tile is done
    DFB::TileDesc *tileDesc = this->getTileDescFor(msg->coords);
    TileData *td = (TileData*)tileDesc;
    this->tileIsCompleted(td);
  }

  void DFB::processMessage(MasterTileMessage_RGBA_I8 *msg)
  {
    if (hasVarianceBuffer && (accumId & 1) == 1)
      tileErrorBuffer[getTileIDof(msg->coords)] = msg->error;
    for (int iy = 0; iy < TILE_SIZE; iy++) {
      int iiy = iy+msg->coords.y;
      if (iiy >= numPixels.y) continue;

      for (int ix = 0; ix < TILE_SIZE; ix++) {
        int iix = ix+msg->coords.x;
        if (iix >= numPixels.x) continue;

        ((uint32*)localFBonMaster->colorBuffer)[iix + iiy*numPixels.x]
          = msg->color[iy][ix];
      }
    }

    // and finally, tell the master that this tile is done
    DFB::TileDesc *tileDesc = this->getTileDescFor(msg->coords);
    TileData *td = (TileData*)tileDesc;
    this->tileIsCompleted(td);
  }

  void DFB::processMessage(MasterTileMessage_RGBA_F32 *msg)
  {
    if (hasVarianceBuffer && (accumId & 1) == 1)
      tileErrorBuffer[getTileIDof(msg->coords)] = msg->error;

    for (int iy = 0; iy < TILE_SIZE; iy++) {
      int iiy = iy+msg->coords.y;
      if (iiy >= numPixels.y)
        continue;

      for (int ix = 0; ix < TILE_SIZE; ix++) {
        int iix = ix+msg->coords.x;
        if (iix >= numPixels.x)
          continue;

        ((vec4f*)localFBonMaster->colorBuffer)[iix + iiy*numPixels.x]
          = msg->color[iy][ix];
      }
    }

    // and finally, tell the master that this tile is done
    DFB::TileDesc *tileDesc = this->getTileDescFor(msg->coords);
    TileData *td = (TileData*)tileDesc;
    this->tileIsCompleted(td);
  }

  void DFB::tileIsCompleted(TileData *tile)
  {
    DBG(printf("rank %i: tilecompleted %i,%i\n",mpi::world.rank,
               tile->begin.x,tile->begin.y));
    if (IamTheMaster()) {
      size_t numTilesCompletedByMyTile = 0;
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
                   tile->begin.x,tile->begin.y,numTilesCompletedThisFrame,
                   numTiles.x*numTiles.y));
      }

      if (numTilesCompletedByMe == myTiles.size())
        closeCurrentFrame();
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

    async([=]() {
      switch (_msg->command) {
      case DFB::MASTER_WRITE_TILE_NONE:
        this->processMessage((DFB::MasterTileMessage_NONE*)_msg);
        break;
      case DFB::MASTER_WRITE_TILE_I8:
        this->processMessage((DFB::MasterTileMessage_RGBA_I8*)_msg);
        break;
      case DFB::MASTER_WRITE_TILE_F32:
        this->processMessage((DFB::MasterTileMessage_RGBA_F32*)_msg);
        break;
      case DFB::WORKER_WRITE_TILE:
        this->processMessage((DFB::WriteTileMessage*)_msg);
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
    doneCond.notify_all();
  }

  //! write given tile data into the frame buffer, sending to remove owner if
  //! required
  void DFB::setTile(ospray::Tile &tile)
  {
    DFB::TileDesc *tileDesc = this->getTileDescFor(tile.region.lower);

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
    if (!myTiles.empty()) {
      parallel_for(myTiles.size(), [&](int taskIndex){
        DFB::TileData *td = this->myTiles[taskIndex];
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
      accumId = -1; // we increment at the start of the frame

      // always also clear error buffer (if present)
      if (tileErrorBuffer) {
        const int tiles = numTiles.x*numTiles.y;
        for (int i = 0; i < tiles; i++)
          tileErrorBuffer[i] = inf;
      }
    }
  }

  float DFB::tileError(const vec2i &tile)
  {
    if (tileErrorBuffer)
      return tileErrorBuffer[getTileIDof(tile)];
    else
      return inf;
  }

  float DFB::endFrame(const float /*errorThreshold*/)
  {
    if (tileErrorBuffer) {
      float maxErr = 0.0f;
      const int tiles = numTiles.x*numTiles.y;
      for (int i = 0; i < tiles; i++)
        maxErr = std::max(maxErr, tileErrorBuffer[i]);

      return maxErr;
    } else
      return inf;
  }

} // ::ospray
