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

namespace ospray {
  using std::cout;
  using std::endl;

  inline int clientRank(int clientID) { return clientID+1; }

  // //! create a new distributd frame buffer with given size and comm handle
  // TiledFrameBuffer *createDistributedFrameBuffer(mpi::async::CommLayer *comm, 
  //                                                const vec2i &numPixels, 
  //                                                size_t handle)
  // {
  //   return new DistributedFrameBuffer(comm,numPixels,handle);
  // }

  DistributedFrameBuffer::Tile::Tile(DistributedFrameBuffer *fb, 
                                                const vec2i &begin, 
                                                size_t tileID, 
                                                size_t ownerID, 
                                                TileData *data)
    : tileID(tileID), ownerID(ownerID), fb(fb), begin(begin), data(data)
  {
  }
    
  //! depth-composite additional 'source' info into existing 'target' tile
  inline void DistributedFrameBuffer
  ::depthComposite(DistributedFrameBuffer::TileData *target,
                   DistributedFrameBuffer::TileData *source)
  {
    const size_t numPixels = TILE_SIZE*TILE_SIZE;
    for (size_t i=0;i<numPixels;i++) {
      if (source->depth[0][i] < target->depth[0][i]) {
        target->depth[0][i] = source->depth[0][i];
        target->color[0][i] = source->color[0][i];
      }
    }
  }
    
  inline void DistributedFrameBuffer::startNewFrame()
  {
    mutex.lock();
    frameIsActive = true;
    // should actually move this to a thread:
    for (int i=0;i<delayedTile.size();i++) {
      WriteTileMessage *msg = delayedTile[i];
      this->incoming(msg);
      // this->writeTile(msg->coords.x,msg->coords.y,TILE_SIZE,
      //                 &msg->tile.color[0][0],&msg->tile.depth[0][0]);
      // delete msg;
    }
    delayedTile.clear();
    mutex.unlock();
  }

  DistributedFrameBuffer
  ::DistributedFrameBuffer(mpi::async::CommLayer *comm,
                           const vec2i &numPixels,
                           size_t myID)
    : mpi::async::CommLayer::Object(comm,myID),
      numPixels(numPixels), maxValidPixelID(numPixels-vec2i(1)),
      numTiles((numPixels.x+TILE_SIZE-1)/TILE_SIZE,
               (numPixels.y+TILE_SIZE-1)/TILE_SIZE),
      frameIsActive(false), frameIsDone(false)
  {
    comm->registerObject(this,myID);
    size_t tileID=0;
    for (size_t y=0;y<numPixels.y;y+=TILE_SIZE)
      for (size_t x=0;x<numPixels.x;x+=TILE_SIZE,tileID++) {
        size_t ownerID = tileID%comm->group->size-1;
        TileData *td = NULL;
        if (clientRank(ownerID) == comm->group->rank) td = new TileData;
        Tile *t = new Tile(this,vec2i(x,y),tileID,ownerID,td);
        tile.push_back(t);
        if (td) 
          myTile.push_back(t);
      }
  }
    
  void DistributedFrameBuffer::incoming(mpi::async::CommLayer::Message *_msg)
  {
    // printf("tiled frame buffer on rank %i received command %li\n",comm->myRank,_msg->command);
    if (_msg->command == this->COMMAND_WRITE_TILE) {
      // start a new frame!
      WriteTileMessage *msg = (WriteTileMessage *)_msg;
      // PRINT(msg->coords);

      printf("client %i RECEIVES tile %i,%i\n",comm->rank(),msg->coords.x,msg->coords.y);
        

      if (!frameIsActive) {
        mutex.lock();
        if (!frameIsActive) {
          // frame is really not yet active - put the tile into the
          // delayed processing buffer, and return WITHOUT deleting
          // it.
          // printf("rank %i DELAYED TILE\n",comm->myRank);
          delayedTile.push_back(msg);
          mutex.unlock();
          return;
        }
        mutex.unlock();
      }

      // printf("rank %i wrote tile %i,%i\n",comm->myRank,
      // msg->coords.x,msg->coords.y);
      
      // "unpack" tile
      ospray::Tile unpacked;
      memcpy(unpacked.r,msg->r,4*TILE_SIZE*TILE_SIZE*sizeof(float));
      unpacked.region.lower = msg->coords;
      unpacked.region.upper = min((msg->coords+vec2i(TILE_SIZE)),getNumPixels());
      
      // this->writeTile(msg->coords.x,msg->coords.y,TILE_SIZE,
      //                 &msg->tile.color[0][0],&msg->tile.depth[0][0]);
      delete msg;
      return;
    }

    throw std::runtime_error("unkown command");
  }

  //! write given tile data into the frame buffer, sending to remove owner if required
  void DistributedFrameBuffer::writeTile(ospray::Tile &tile)
  { 
    const size_t numPixels = TILE_SIZE*TILE_SIZE;
    const int x0 = tile.region.lower.x;
    const int y0 = tile.region.lower.y;
    typename DistributedFrameBuffer::Tile *myTile
      = this->getTileFor(x0,y0);
    if (myTile->data == NULL) {
      // NOT my tile...
      WriteTileMessage *msg = new WriteTileMessage;
      msg->coords = vec2i(x0,y0);
      // TODO: compress pixels before sending ...
      memcpy(msg->r,tile.r,4*numPixels*sizeof(float));
      // memcpy(msg->r,tile.r,numPixels*sizeof(float));
      // memcpy(msg->g,tile.g,numPixels*sizeof(float));
      // memcpy(msg->b,tile.b,numPixels*sizeof(float));
      // memcpy(msg->a,tile.a,numPixels*sizeof(float));
        
      // size_t pixelID = 0;
      // TileData *td = &msg->tile;
      // for (size_t dy=0;dy<TILE_SIZE;dy++)
      //   for (size_t dx=0;dx<TILE_SIZE;dx++, pixelID++) {
      //     const size_t x = std::min(x0+dx,(size_t)maxValidPixelID.x);
      //     const size_t y = std::min(y0+dy,(size_t)maxValidPixelID.y);
      //     td->color[0][pixelID] = colorChannel[dx+dy*dxdy];
      //     td->depth[0][pixelID] = depthChannel[dx+dy*dxdy];
      //   }
      // msg->sourceHandle = myHandle;
      // msg->targetHandle = myHandle;
      msg->command      = COMMAND_WRITE_TILE;
      // printf("client %i SENDS tile %li,%li\n",comm->rank(),x0,y0);
        
      comm->sendTo(mpi::async::CommLayer::Address(clientRank(myTile->ownerID),myID),
                   msg,sizeof(*msg));
    } else {
      // this is my tile...

      // printf("client %i WRITES tile %li,%li\n",comm->rank(),x0,y0);

      // typename DistributedFrameBuffer::TileData td;
      size_t pixelID = 0;
      for (size_t dy=0;dy<TILE_SIZE;dy++)
        for (size_t dx=0;dx<TILE_SIZE;dx++, pixelID++) {
          memcpy(myTile->data->accum_r,tile.r,4*TILE_SIZE*TILE_SIZE*sizeof(float));
          // const size_t x = std::min(x0+dx,(size_t)maxValidPixelID.x);
          // const size_t y = std::min(y0+dy,(size_t)maxValidPixelID.y);
          // td.color[0][pixelID] = colorChannel[dx+dy*dxdy];
          // td.depth[0][pixelID] = depthChannel[dx+dy*dxdy];
        }
      assert(frameIsActive);
      // DistributedFrameBuffer::depthComposite(tile->data,&td);
    }
    // delete td;
  }

  // TiledFrameBuffer::DistributedFrameBuffer(mpi::async::CommLayer *comm,
  //                                    size_t myID) 
  //   : mpi::async::CommLayer::Object(comm,myID)
  // {}

}
