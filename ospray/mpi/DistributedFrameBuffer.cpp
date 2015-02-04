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
  namespace dpr {
    using std::cout;
    using std::endl;

    inline int clientRank(int clientID) { return clientID+1; }

    //! create a new distributd frame buffer with given size and comm handle
    TiledFrameBuffer *createDistributedFrameBuffer(mpi::async::CommLayer *comm, 
                                                   const vec2i &numPixels, 
                                                   size_t handle)
    {
      return new DistributedFrameBuffer<128>(comm,numPixels,handle);
    }

    template<int TILE_SIZE>
    DistributedFrameBuffer<TILE_SIZE>::Tile::Tile(DistributedFrameBuffer *fb, 
                                                  const vec2i &begin, 
                                                  size_t tileID, 
                                                  size_t ownerID, 
                                                  TileData *data)
      : tileID(tileID), ownerID(ownerID), fb(fb), begin(begin), data(data)
    {
    }
    
    //! depth-composite additional 'source' info into existing 'target' tile
    template<int TILE_SIZE>
    inline void DistributedFrameBuffer<TILE_SIZE>
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
    
    template<int TILE_SIZE>
    inline void DistributedFrameBuffer<TILE_SIZE>::startNewFrame()
    {
      mutex.lock();
      frameIsActive = true;
      // should actually move this to a thread:
      for (int i=0;i<delayedTile.size();i++) {
        WriteTileMessage *msg = delayedTile[i];
        this->writeTile(msg->coords.x,msg->coords.y,TILE_SIZE,
                        &msg->tile.color[0][0],&msg->tile.depth[0][0]);
        delete msg;
      }
      delayedTile.clear();
      mutex.unlock();
    }

    template<int TILE_SIZE>
    DistributedFrameBuffer<TILE_SIZE>
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
    
    template<int TILE_SIZE>
    void DistributedFrameBuffer<TILE_SIZE>
    ::incoming(mpi::async::CommLayer::Message *_msg)
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
        this->writeTile(msg->coords.x,msg->coords.y,TILE_SIZE,
                        &msg->tile.color[0][0],&msg->tile.depth[0][0]);
        delete msg;
        return;
      }

      throw std::runtime_error("unkown command");
    }

    //! write given tile data into the frame buffer, sending to remove owner if required
    template<int TILE_SIZE>
    void DistributedFrameBuffer<TILE_SIZE>::writeTile(size_t x0, size_t y0, size_t dxdy,
                                                      uint32 *colorChannel, float *depthChannel)
    { 
      const size_t numPixels = TILE_SIZE*TILE_SIZE;
      
      typename DistributedFrameBuffer<TILE_SIZE>::TileData *td = new typename DistributedFrameBuffer<TILE_SIZE>::TileData;
        
      // first, read the tile
        
      typename DistributedFrameBuffer<TILE_SIZE>::Tile *tile = this->getTileFor(x0,y0);
      if (tile->data == NULL) {
        // NOT my tile...
        WriteTileMessage *msg = new WriteTileMessage;
        msg->coords = vec2i(x0,y0);
        
        size_t pixelID = 0;
        TileData *td = &msg->tile;
        for (size_t dy=0;dy<TILE_SIZE;dy++)
          for (size_t dx=0;dx<TILE_SIZE;dx++, pixelID++) {
            const size_t x = std::min(x0+dx,(size_t)maxValidPixelID.x);
            const size_t y = std::min(y0+dy,(size_t)maxValidPixelID.y);
            td->color[0][pixelID] = colorChannel[dx+dy*dxdy];
            td->depth[0][pixelID] = depthChannel[dx+dy*dxdy];
          }
        // msg->sourceHandle = myHandle;
        // msg->targetHandle = myHandle;
        msg->command      = COMMAND_WRITE_TILE;
        printf("client %i SENDS tile %li,%li\n",comm->rank(),x0,y0);
        
        comm->sendTo(mpi::async::CommLayer::Address(clientRank(tile->ownerID),myID),
                     msg,sizeof(*msg));
      } else {
        // this is my tile...

        printf("client %i WRITES tile %li,%li\n",comm->rank(),x0,y0);

        typename DistributedFrameBuffer<TILE_SIZE>::TileData td;
        size_t pixelID = 0;
        for (size_t dy=0;dy<TILE_SIZE;dy++)
          for (size_t dx=0;dx<TILE_SIZE;dx++, pixelID++) {
            const size_t x = std::min(x0+dx,(size_t)maxValidPixelID.x);
            const size_t y = std::min(y0+dy,(size_t)maxValidPixelID.y);
            td.color[0][pixelID] = colorChannel[dx+dy*dxdy];
            td.depth[0][pixelID] = depthChannel[dx+dy*dxdy];
          }
        assert(frameIsActive);
        DistributedFrameBuffer<TILE_SIZE>::depthComposite(tile->data,&td);
        if (tileWrittenCBFunc)
          tileWrittenCBFunc(this,tileWrittenCBData);
      }
      delete td;
    }

    void TiledFrameBuffer::setTileWrittenCB(TileWrittenCB func, void *data)
    { 
      tileWrittenCBFunc = func; 
      tileWrittenCBData = data; 
    }

    TiledFrameBuffer::TiledFrameBuffer() 
      : tileWrittenCBData(NULL), 
        tileWrittenCBFunc(NULL)
    {}

    //    template struct DistributedFrameBuffer<128>;
  }
}
