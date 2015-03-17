// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#include "ospray/common/TaskSys.h"

namespace ospray {
  using std::cout;
  using std::endl;

  inline int clientRank(int clientID) { return clientID+1; }

  typedef DistributedFrameBuffer DFB;

  DFB::TileDesc::TileDesc(DFB *dfb, 
                          const vec2i &begin, 
                          size_t tileID, 
                          size_t ownerID)
    : tileID(tileID), ownerID(ownerID), dfb(dfb), begin(begin)
  {
  }

  DFB::TileData::TileData(DFB *dfb, 
                          const vec2i &begin, 
                          size_t tileID, 
                          size_t ownerID) 
    : TileDesc(dfb,begin,tileID,ownerID) 
  {}
  
  /*! called exactly once at the beginning of each frame */
  void DFB::WriteOnlyOnceTile::newFrame()
  { 
    /* nothing to do for write-once tile - we *know* it'll get written
       only once */ 
  }

  void DFB::WriteOnlyOnceTile::process(const ospray::Tile &tile)
  {
    bool debug = 0;
    ispc::DFB_accumTile((ispc::VaryingTile *)&tile,
                        (ispc::VaryingTile*)&this->accum,
                        (ispc::VaryingRGBA_I8*)&this->color,
                        dfb->hasAccumBuffer,dfb->accumID,debug);
    dfb->tileIsCompleted(this);
  }


  inline void DFB::startNewFrame()
  {
    mutex.lock();
    assert(!frameIsActive);

    for (int i=0;i<myTiles.size();i++) 
      myTiles[i]->newFrame();
    // #if MPI_IMAGE_COMPOSITING
    //     for (int i=0;i<myTile.size();i++) 
    //       myTile[i]->data->numTimesWrittenThisFrame = 0;
    // #endif

    // create a local copy of delayed tiles, so we can work on them outside the mutex
    std::vector<mpi::async::CommLayer::Message *> 
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

    mutex.unlock();
    
    // might actually want to move this to a thread:
    for (int i=0;i<delayedMessage.size();i++) {
      mpi::async::CommLayer::Message *msg = delayedMessage[i];
      this->incoming(msg);
    }
  }

  DFB::DistributedFrameBuffer(mpi::async::CommLayer *comm,
                              const vec2i &numPixels,
                              size_t myID,
                              ColorBufferFormat colorBufferFormat,
                              bool hasDepthBuffer,
                              bool hasAccumBuffer)
    : mpi::async::CommLayer::Object(comm,myID),
      FrameBuffer(numPixels,colorBufferFormat,hasDepthBuffer,hasAccumBuffer),
      numPixels(numPixels),
      maxValidPixelID(numPixels-vec2i(1)),
      numTiles((numPixels.x+TILE_SIZE-1)/TILE_SIZE,
               (numPixels.y+TILE_SIZE-1)/TILE_SIZE),
      frameIsActive(false), frameIsDone(false), localFBonMaster(NULL)
  {
    assert(comm);
    this->ispcEquivalent = ispc::DFB_create(this);
    ispc::DFB_set(getIE(),numPixels.x,numPixels.y,
                  colorBufferFormat);
    comm->registerObject(this,myID);
    size_t tileID=0;
    for (size_t y=0;y<numPixels.y;y+=TILE_SIZE)
      for (size_t x=0;x<numPixels.x;x+=TILE_SIZE,tileID++) {
        size_t ownerID = tileID % (comm->group->size-1);
        if (clientRank(ownerID) == comm->group->rank) {
#if MPI_IMAGE_COMPOSITING
          TileData *td = new ZCompositeTile(this,vec2i(x,y),tileID,ownerID);
#else
          TileData *td = new WriteOnlyOnceTile(this,vec2i(x,y),tileID,ownerID);
#endif
          myTiles.push_back(td);
          allTiles.push_back(td);
        } else {
          allTiles.push_back(new TileDesc(this,vec2i(x,y),tileID,ownerID));
        }
      }

    if (comm->group->rank == 0) {
      if (colorBufferFormat == OSP_RGBA_NONE) {
        cout << "#osp:mpi:dfb: we're the master, but framebuffer has 'NONE' format; creating distriubted frame buffer WITHOUT having a mappable copy on the master" << endl;
      } else {
        cout << "#osp:mpi:dfb: we're the master - creating a local fb to gather results" << endl;
        localFBonMaster = new LocalFrameBuffer(numPixels,colorBufferFormat,hasDepthBuffer,0);
      }
    }
    ispc::DFB_set(getIE(),numPixels.x,numPixels.y,
                  colorBufferFormat);
  }

  const void *DFB::mapDepthBuffer() 
  {
    if (!localFBonMaster)
      throw std::runtime_error("#osp:mpi:dfb: tried to 'ospMap()' the depth buffer of a frame buffer buffer that doesn't have a host-side color buffer");
    assert(localFBonMaster);
    return localFBonMaster->mapDepthBuffer();
  }

  const void *DFB::mapColorBuffer() 
  {
    if (!localFBonMaster)
      throw std::runtime_error("#osp:mpi:dfb: tried to 'ospMap()' the color buffer of a frame buffer that doesn't have a host-side color buffer");
    assert(localFBonMaster);
    return localFBonMaster->mapColorBuffer();
  }

  void DFB::unmap(const void *mappedMem) 
  {
    if (!localFBonMaster)
      throw std::runtime_error("#osp:mpi:dfb: tried to 'ospUnmap()' a frame buffer that doesn't have a host-side color buffer");
    assert(localFBonMaster);
    localFBonMaster->unmap(mappedMem);
  }

  void DFB::waitUntilFinished() 
  {
    // PING;
    mutex.lock();
    // PRINT(frameIsDone);
    // printf("rank %i is WAITING\n",mpi::world.rank);
    while (!frameIsDone) 
      doneCond.wait(mutex);
    // printf("rank %i is done\n",mpi::world.rank);
    mutex.unlock();
  }



  void DFB::processMessage(DFB::WriteTileMessage *msg)
  {
    DFB::TileDesc *tileDesc = this->getTileDescFor(msg->coords);
    TileData *td = (TileData*)tileDesc;
    td->process(msg->tile);
    //         // TODO: "unpack" tile
    //         ospray::Tile unpacked;
    //         jmemcpy(unpacked.r,msg->r,TILE_SIZE*TILE_SIZE*sizeof(float));
    //         memcpy(unpacked.g,msg->g,TILE_SIZE*TILE_SIZE*sizeof(float));
    //         memcpy(unpacked.b,msg->b,TILE_SIZE*TILE_SIZE*sizeof(float));
    //         memcpy(unpacked.a,msg->a,TILE_SIZE*TILE_SIZE*sizeof(float));
    // #if MPI_IMAGE_COMPOSITING
    //         memcpy(unpacked.z,msg->z,TILE_SIZE*TILE_SIZE*sizeof(float));
    // #endif
    //         unpacked.region.lower = msg->coords;
    //         unpacked.region.upper = min((msg->coords+vec2i(TILE_SIZE)),dfb->getNumPixels());
      
    //         // cout << "worker RECEIVED tile " << unpacked.region.lower << endl;
    //         dfb->writeTile(unpacked);
    //         // msg->coords.x,msg->coords.y,TILE_SIZE,
    //         //                       &msg->tile.color[0][0],&msg->tile.depth[0][0]);
  }
  
  void DFB::processMessage(MasterTileMessage_NONE *msg)
  {
    /* nothing to do for 'none' tiles */
    {}

    // and finally, tell the master that this tile is done
    DFB::TileDesc *tileDesc = this->getTileDescFor(msg->coords);
    TileData *td = (TileData*)tileDesc;
    this->tileIsCompleted(td);
  }

  void DFB::processMessage(MasterTileMessage_RGBA_I8 *msg)
  {
    for (int iy=0;iy<TILE_SIZE;iy++) {
      int iiy = iy+msg->coords.y;
      if (iiy >= numPixels.y) continue;
      
      for (int ix=0;ix<TILE_SIZE;ix++) {
        int iix = ix+msg->coords.x;
        if (iix >= numPixels.x) continue;
        
        ((uint*)localFBonMaster->colorBuffer)[iix + iiy*numPixels.x] 
          = msg->color[iy][ix];
      }
    }

    // and finally, tell the master that this tile is done
    DFB::TileDesc *tileDesc = this->getTileDescFor(msg->coords);
    TileData *td = (TileData*)tileDesc;
    this->tileIsCompleted(td);
  }


  struct DFBProcessMessageTask : public ospray::Task {
    DFB *dfb;
    mpi::async::CommLayer::Message *_msg;

    DFBProcessMessageTask(DFB *dfb,
                          mpi::async::CommLayer::Message *_msg) 
      : dfb(dfb), _msg(_msg) 
    {}
    
    // void writeAtClient()
    // {
    //   dfb->processMessage((DFB::WriteTileMessage*)_msg);

    // }
    
    // void writeAtMaster()
    // {
    //   switch(dfb->colorBufferFormat) {
    //   case OSP_RGBA_NONE: 
    //     /* nothing to do */
    //     break;
    //   case OSP_RGBA_I8: 
    //     dfb->processMessage((DFB::MasterTileMessage_RGBA_I8 *)_msg);
    //     break;
    //   default:
    //     throw std::runtime_error("#osp:mpi:dfb: color buffer format not implemented "
    //                              "for distributed frame buffer");
    //   };
      
    //   dfb->tileIsCompleted(tile);
    //   // printf("rank %i (MASTER) is completed %li\n",mpi::world.rank,dfb->numTilesCompletedThisFrame);
    //   // dfb->mutex.lock();
    //   // dfb->numTilesCompletedThisFrame++;
    //   // PING;
    //   // PRINT(dfb->numTilesCompletedThisFrame);
    //   // if (dfb->numTilesCompletedThisFrame == dfb->numTiles.x*dfb->numTiles.y)
    //   //   dfb->closeCurrentFrame(true);
    //   // dfb->mutex.unlock();
    //   delete _msg;
    //   return;
    // }

    virtual void run(size_t jobID) 
    {
      switch (_msg->command) {
      case DFB::MASTER_WRITE_TILE_NONE: 
        dfb->processMessage((DFB::MasterTileMessage_NONE*)_msg);
        break;
      case DFB::MASTER_WRITE_TILE_I8: 
        dfb->processMessage((DFB::MasterTileMessage_RGBA_I8*)_msg);
        break;
      case DFB::WORKER_WRITE_TILE:
        dfb->processMessage((DFB::WriteTileMessage*)_msg);
        break;
      default:
        assert(0);
      };
      delete _msg;
      // if (_msg->command == DFB::MASTER_WRITE_TILE) {
      //   writeAtMaster();
      //   switch(dfb->colorBufferFormat) {
      //   case OSP_RGBA_NONE: 
      //     /* nothing to do */
      //     break;
      //   case OSP_RGBA_I8: 
      //     dfb->processMessage((DFB::MasterTileMessage_RGBA_I8 *)_msg);
      //     break;
      //   default:
      //     throw std::runtime_error("#osp:mpi:dfb: color buffer format not implemented "
      //                              "for distributed frame buffer");
      //   };
      //   printf("rank %i (MASTER) is completed %li\n",mpi::world.rank,dfb->numTilesCompletedThisFrame);
      //   dfb->mutex.lock();
      //   dfb->numTilesCompletedThisFrame++;
      //   PING;
      //   PRINT(dfb->numTilesCompletedThisFrame);
      //   if (dfb->numTilesCompletedThisFrame == dfb->numTiles.x*dfb->numTiles.y)
      //     dfb->closeCurrentFrame(true);
      //   dfb->mutex.unlock();
      //   delete _msg;
      //   return;
      // }

      // if (_msg->command == DFB::WORKER_WRITE_TILE) {
      //   DFB::WriteTileMessage *msg
      //     = (DFB::WriteTileMessage *)_msg;
      //   dfb->processMessage(msg);

      //         // TODO: "unpack" tile
      //         ospray::Tile unpacked;
      //         jmemcpy(unpacked.r,msg->r,TILE_SIZE*TILE_SIZE*sizeof(float));
      //         memcpy(unpacked.g,msg->g,TILE_SIZE*TILE_SIZE*sizeof(float));
      //         memcpy(unpacked.b,msg->b,TILE_SIZE*TILE_SIZE*sizeof(float));
      //         memcpy(unpacked.a,msg->a,TILE_SIZE*TILE_SIZE*sizeof(float));
      // #if MPI_IMAGE_COMPOSITING
      //         memcpy(unpacked.z,msg->z,TILE_SIZE*TILE_SIZE*sizeof(float));
      // #endif
      //         unpacked.region.lower = msg->coords;
      //         unpacked.region.upper = min((msg->coords+vec2i(TILE_SIZE)),dfb->getNumPixels());
      
      //         // cout << "worker RECEIVED tile " << unpacked.region.lower << endl;
      //         dfb->writeTile(unpacked);
      //         // msg->coords.x,msg->coords.y,TILE_SIZE,
      //         //                       &msg->tile.color[0][0],&msg->tile.depth[0][0]);
      // delete msg;
      // return;
    }
  };

  void DFB::tileIsCompleted(TileData *tile)
  {
    // printf("rank %i: tilecompleted %i,%i\n",mpi::world.rank,
    //        tile->begin.x,tile->begin.y);
    if (IamTheMaster()) {
      /*! we will not do anything with the tile other than mark it's done */
    } else {
      if (pixelOp)
        pixelOp->postAccum(tile->accum);
    
      // MasterTileMessage *mtm = new MasterTileMessage;
      // mtm->command = MASTER_WRITE_TILE;
      // mtm->coords  = myTile->begin;
      // memcpy(mtm->color,td->color,TILE_SIZE*TILE_SIZE*sizeof(uint32));
      // comm->sendTo(this->master,mtm,sizeof(*mtm));
      
      switch(colorBufferFormat) {
      case OSP_RGBA_NONE: {
        MasterTileMessage_NONE *mtm = new MasterTileMessage_NONE;
        mtm->command = MASTER_WRITE_TILE_NONE;
        mtm->coords  = tile->begin;
        // printf("rank sending to master %i,%i\n",mtm->coords.x,mtm->coords.y);
        comm->sendTo(this->master,mtm,sizeof(*mtm));
      } break;
      case OSP_RGBA_I8: {
        MasterTileMessage_RGBA_I8 *mtm = new MasterTileMessage_RGBA_I8;
        mtm->command = MASTER_WRITE_TILE_I8;
        mtm->coords  = tile->begin;
        memcpy(mtm->color,tile->color,TILE_SIZE*TILE_SIZE*sizeof(uint32));
        // printf("rank sending to master %i,%i\n",mtm->coords.x,mtm->coords.y);
        comm->sendTo(this->master,mtm,sizeof(*mtm));
      } break;
      default:
        throw std::runtime_error("#osp:mpi:dfb: color buffer format not implemented for distributed frame buffer");
      };
        
      // #if 0
      //     char fn[1000];
      //     sprintf(fn,"/tmp/dfb_seq%05i_color_tile_%04i_%04i.ppm",accumID+1,
      //             myTile->begin.x,myTile->begin.y);
      //     vec2i size(TILE_SIZE,TILE_SIZE);
      //     writePPM(fn,size,(uint32*)&myTile->data->color);
      // #endif

      // dfb->tileIsCompleted(tile);
    }

    // finally, do the book-keeping that this tile is completely done.

    mutex.lock();
    numTilesCompletedThisFrame++;
    // printf("rank %i: MARKING AS COMPLETED %i,%i -> %i %i\n",mpi::world.rank,
    //        tile->begin.x,tile->begin.y,numTilesCompletedThisFrame,numTiles.x*numTiles.y);
    if (numTilesCompletedThisFrame == numTiles.x*numTiles.y)
      closeCurrentFrame(true);
    mutex.unlock();
  }
  
  void DFB::incoming(mpi::async::CommLayer::Message *_msg)
  {
    // printf("incoming at %i: %i\n",mpi::world.rank,_msg->command);
    if (!frameIsActive) {
      mutex.lock();
      if (!frameIsActive) {
        // frame is really not yet active - put the tile into the
        // delayed processing buffer, and return WITHOUT deleting
        // it.
        delayedMessage.push_back(_msg);
        // printf("DELAYING at %i: %i\n",mpi::world.rank,_msg->command);
        mutex.unlock();
        return;
      }
      mutex.unlock();
    }

    // #if 1
    Ref<DFBProcessMessageTask> msgTask = new DFBProcessMessageTask(this,_msg);
    msgTask->schedule(1);
    // #else

    //     // printf("tiled frame buffer on rank %i received command %li\n",comm->myRank,_msg->command);
    //     if (_msg->command == MASTER_WRITE_TILE) {
    //       switch(colorBufferFormat) {
    //       case OSP_RGBA_NONE: 
    //         /* nothing to do */
    //         break;
    //       case OSP_RGBA_I8: 
    //         writeTile((MasterTileMessage_RGBA_I8 *)_msg);
    //         break;
    //       default:
    //         throw std::runtime_error("#osp:mpi:dfb: color buffer format not implemented "
    //                                  "for distributed frame buffer");
    //       };
    //       dfb->tileIsCompleted(tileThatJustCompleted);
    //       // mutex.lock();
    //       // numTilesWrittenThisFrame++;
    //       // if (numTilesWrittenThisFrame == numTiles.x*numTiles.y)
    //       //   closeCurrentFrame(true);
    //       // mutex.unlock();
    //       delete _msg;
    //       return;
    //     }

    //     if (_msg->command == WORKER_WRITE_TILE) {
    //       WriteTileMessage *msg = (WriteTileMessage *)_msg;

    //       // TODO: "unpack" tile
    //       ospray::Tile unpacked;
    //       memcpy(unpacked.r,msg->r,TILE_SIZE*TILE_SIZE*sizeof(float));
    //       memcpy(unpacked.g,msg->g,TILE_SIZE*TILE_SIZE*sizeof(float));
    //       memcpy(unpacked.b,msg->b,TILE_SIZE*TILE_SIZE*sizeof(float));
    //       memcpy(unpacked.a,msg->a,TILE_SIZE*TILE_SIZE*sizeof(float));
    // #if MPI_IMAGE_COMPOSITING
    //       memcpy(unpacked.z,msg->z,TILE_SIZE*TILE_SIZE*sizeof(float));
    // #endif
    //       unpacked.region.lower = msg->coords;
    //       unpacked.region.upper = min((msg->coords+vec2i(TILE_SIZE)),getNumPixels());
      
    //       // cout << "worker RECEIVED tile " << unpacked.region.lower << endl;
    //       this->writeTile(unpacked);
    // // msg->coords.x,msg->coords.y,TILE_SIZE,
    // //                       &msg->tile.color[0][0],&msg->tile.depth[0][0]);
    //       delete msg;
    //       return;
    //     }

    //     throw std::runtime_error("#osp:mpi:DFB: unknown command");
    // #endif
      }

  // void DFB::setTile(ospray::Tile &tile)
  // {
  //   writeTile(tile);
  // }

  void DFB::closeCurrentFrame(bool locked) {
    if (!locked) mutex.lock();
    frameIsActive = false;
    frameIsDone   = true;
    doneCond.broadcast();

    if (!locked) mutex.unlock();
  };

  //! write given tile data into the frame buffer, sending to remove owner if required
  void DFB::setTile(ospray::Tile &tile)
  { 
    const size_t numPixels = TILE_SIZE*TILE_SIZE;
    // const int x0 = tile.region.lower.x;
    // const int y0 = tile.region.lower.y;
    DFB::TileDesc *tileDesc = this->getTileDescFor(tile.region.lower);

    //    printf("found tile %lx data %lx\n",myTile,myTile->data);
    if (!tileDesc->mine()) {
      // NOT my tile...
      WriteTileMessage *msg = new WriteTileMessage;
      msg->coords = tile.region.lower;
      // TODO: compress pixels before sending ...
      memcpy(&msg->tile,&tile,sizeof(ospray::Tile));
      // memcpy(msg->r,tile.r,TILE_SIZE*TILE_SIZE*sizeof(float));
      // memcpy(msg->g,tile.g,TILE_SIZE*TILE_SIZE*sizeof(float));
      // memcpy(msg->b,tile.b,TILE_SIZE*TILE_SIZE*sizeof(float));
      // memcpy(msg->a,tile.a,TILE_SIZE*TILE_SIZE*sizeof(float));
      // memcpy(msg->z,tile.z,TILE_SIZE*TILE_SIZE*sizeof(float));
      msg->command      = WORKER_WRITE_TILE;

      comm->sendTo(this->worker[tileDesc->ownerID],
                   msg,sizeof(*msg));
    } else {
      // this is my tile...
      assert(frameIsActive);

      // if (pixelOp)
      //   pixelOp->preAccum(tile);

      //! accumulate new tile data with existing accum data for this tile
      // TileData *td = (TileData*)myTile->data;
      TileData *td = (TileData*)tileDesc;
      td->process(tile);


      //       if (!td) 
      //         throw std::runtime_error("not my tile!");

      // #if MPI_IMAGE_COMPOSITING
      //       td->mutex.lock();
      //       {
      //         size_t pixelID = 0;
      //         if (td->numTimesWrittenThisFrame == 0) {
      //           for (size_t iy=0;iy<TILE_SIZE;iy++)
      //             for (size_t ix=0;ix<TILE_SIZE;ix++,pixelID++) {
      //               td->comp_r[iy][ix] = tile.r[pixelID];
      //               td->comp_g[iy][ix] = tile.g[pixelID];
      //               td->comp_b[iy][ix] = tile.b[pixelID];
      //               td->comp_z[iy][ix] = tile.z[pixelID];
      //             }
      //         } else {
      //           for (size_t iy=0;iy<TILE_SIZE;iy++)
      //             for (size_t ix=0;ix<TILE_SIZE;ix++,pixelID++) {
      //               bool closer = tile.z[pixelID] < td->comp_z[iy][ix];
      //               td->comp_r[iy][ix] = closer ? tile.r[pixelID] : td->comp_r[iy][ix];
      //               td->comp_g[iy][ix] = closer ? tile.g[pixelID] : td->comp_g[iy][ix];
      //               td->comp_b[iy][ix] = closer ? tile.b[pixelID] : td->comp_b[iy][ix];
      //               td->comp_z[iy][ix] = closer ? tile.z[pixelID] : td->comp_z[iy][ix];
      //             }
      //         }
      //       }
      //       td->numTimesWrittenThisFrame++;
      //       // PRINT(td->numTimesWrittenThisFrame);
      //       // PRINT(comm->numWorkers());
      //       if (td->numTimesWrittenThisFrame == comm->numWorkers()) {
      //         // we're the last one to write. let's just write that data
      //         // back into the tile, and pretend that nothign happened :-)
      //         size_t pixelID = 0;
      //         for (size_t iy=0;iy<TILE_SIZE;iy++)
      //           for (size_t ix=0;ix<TILE_SIZE;ix++,pixelID++) {
      //             tile.r[pixelID] = td->comp_r[iy][ix];
      //             tile.g[pixelID] = td->comp_g[iy][ix];
      //             tile.b[pixelID] = td->comp_b[iy][ix];
      //             tile.a[pixelID] = 1.f;
      //           }
      //       } else {
      //         // there's more that want to write before this tile is
      //         // finished. return here, and wait for somebody else to
      //         // complet this tile.
      //         td->mutex.unlock();
      //         return;
      //       }
      //       td->mutex.unlock();
      // #endif

      //       {
      // #if 1
      //         ispc::DFB_accumTile((ispc::VaryingTile *)&tile,(ispc::DFBTileData*)td,hasAccumBuffer,accumID);
      // #else
      //         // perform tile accumulation. TODO: do this in ISPC
      //         size_t pixelID = 0;
      //         for (size_t iy=0;iy<TILE_SIZE;iy++)
      //           for (size_t ix=0;ix<TILE_SIZE;ix++, pixelID++) {
      //             float rcpAccumID = 1.f/(accumID+1);
      //             vec4f col = vec4f(tile.r[pixelID],
      //                               tile.g[pixelID],
      //                               tile.b[pixelID],
      //                               tile.a[pixelID]);
      //             vec4f old_col = col;
      //             vec4f old_accum_col = vec4f(-1.f);
      //             if (hasAccumBuffer) {
      //               if (accumID > 0) {
      //                 old_accum_col = vec4f(td->accum_r[iy][ix],
      //                              td->accum_g[iy][ix],
      //                              td->accum_b[iy][ix],
      //                              td->accum_a[iy][ix]);
      //                 col += vec4f(td->accum_r[iy][ix],
      //                              td->accum_g[iy][ix],
      //                              td->accum_b[iy][ix],
      //                              td->accum_a[iy][ix]);
      //               }
      //               td->accum_r[iy][ix] = col.x;
      //               td->accum_g[iy][ix] = col.y;
      //               td->accum_b[iy][ix] = col.z;
      //               td->accum_a[iy][ix] = col.w;
      //               col *= rcpAccumID;
      //             }
      //             // if (fabsf(col.x-1.f) > 1e-5f) { 
      //             //   PRINT(hasAccumBuffer);
      //             //   PRINT(accumID);
      //             //   PRINT(old_accum_col);
      //             //   PRINT(old_col);
      //             //   PRINT(rcpAccumID);
      //             //   PRINT(col);
      //             // }
      //             td->color[iy][ix] = cvt_uint32(col);
      //           }
      // #endif
      //       }

      // if (pixelOp)
      //   pixelOp->postAccum(tile);

      // MasterTileMessage *mtm = new MasterTileMessage;
      // mtm->command = MASTER_WRITE_TILE;
      // mtm->coords  = myTile->begin;
      // memcpy(mtm->color,td->color,TILE_SIZE*TILE_SIZE*sizeof(uint32));
      // comm->sendTo(this->master,mtm,sizeof(*mtm));
      
      //       switch(colorBufferFormat) {
      //       case OSP_RGBA_NONE: {
      //         MasterTileMessage_NONE *mtm = new MasterTileMessage_NONE;
      //         mtm->command = MASTER_WRITE_TILE;
      //         mtm->coords  = myTile->begin;
      //         comm->sendTo(this->master,mtm,sizeof(*mtm));
      //       } break;
      //       case OSP_RGBA_I8: {
      //         MasterTileMessage_RGBA_I8 *mtm = new MasterTileMessage_RGBA_I8;
      //         mtm->command = MASTER_WRITE_TILE;
      //         mtm->coords  = myTile->begin;
      //         memcpy(mtm->color,td->color,TILE_SIZE*TILE_SIZE*sizeof(uint32));
      //         comm->sendTo(this->master,mtm,sizeof(*mtm));
      //       } break;
      //       default:
      //         throw std::runtime_error("#osp:mpi:dfb: color buffer format not implemented for distributed frame buffer");
      //       };
        
      // #if 0
      //       char fn[1000];
      //       sprintf(fn,"/tmp/dfb_seq%05i_color_tile_%04i_%04i.ppm",accumID+1,
      //               myTile->begin.x,myTile->begin.y);
      //       vec2i size(TILE_SIZE,TILE_SIZE);
      //       writePPM(fn,size,(uint32*)&myTile->data->color);
      // #endif

      //       dfb->tileIsCompleted(tile);
      //       // mutex.lock();
      //       // numTilesWrittenThisFrame++;
      //       // // printf("CLIENT NUM WRITTEN %i\n",numTilesWrittenThisFrame);
      //       // if (numTilesWrittenThisFrame == numMyTiles())
      //       //   closeCurrentFrame(true);

      //       mutex.unlock();
    }
  }

  struct DFBClearTask : public ospray::Task {
    DFB *dfb;
    DFBClearTask(DFB *dfb) : dfb(dfb) {};
    virtual void run(size_t jobID) 
    {
      size_t tileID = jobID;
      DFB::TileData *td = dfb->myTiles[tileID];
      assert(td);
      for (int i=0;i<TILE_SIZE*TILE_SIZE;i++) td->accum.r[i] = 0.f;
      for (int i=0;i<TILE_SIZE*TILE_SIZE;i++) td->accum.g[i] = 0.f;
      for (int i=0;i<TILE_SIZE*TILE_SIZE;i++) td->accum.b[i] = 0.f;
      for (int i=0;i<TILE_SIZE*TILE_SIZE;i++) td->accum.a[i] = 0.f;
      // for (int i=0;i<TILE_SIZE*TILE_SIZE;i++) 
      //   (&td->accum.g[0][0])[i] = 0.f;
      // for (int i=0;i<TILE_SIZE*TILE_SIZE;i++) 
      //     (&td->accum_b[0][0])[i] = 0.f;
      // for (int i=0;i<TILE_SIZE*TILE_SIZE;i++) 
      //   (&td->accum_a[0][0])[i] = 0.f;
    }
  };

  /*! \brief clear (the specified channels of) this frame buffer 
      
    \details for the *distributed* frame buffer, we assume that
    *all* nodes get this command, and that each instance therefore
    can clear only its own tiles without having to tell any other
    node about it
  */
  void DFB::clear(const uint32 fbChannelFlags)
  {
    if (fbChannelFlags & OSP_FB_ACCUM) {
      Ref<DFBClearTask> clearTask = new DFBClearTask(this);
      clearTask->schedule(myTiles.size());
      clearTask->wait();
      accumID = 0;
    }
  }

}
