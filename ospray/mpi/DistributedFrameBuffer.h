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

#pragma once

#include "ospray/mpi/async/CommLayer.h"

namespace ospray {
  using std::cout;
  using std::endl;

  //! virtual abstraction for a tiled frame buffer with fixed tile size
  struct TiledFrameBuffer : public mpi::async::CommLayer::Object
  {
    typedef void (*TileWrittenCB)(TiledFrameBuffer *which, void *data);
      
    /*! set a call back function (and a pointer data) to be called
      whenever the farme is finished (on that client). Function
      will get called when this client has written all its data
      (including those that were remotely written by remote
      nodes); that does not mean, though, that all of our peers
      have already finished yet) */
    void setTileWrittenCB(TileWrittenCB func, void *data);

    TiledFrameBuffer(mpi::async::CommLayer *comm,size_t myID);

    //! get number of pixels per tile, in x and y direction
    virtual vec2i getTileSize() const = 0;
    //! return number of tiles in x and y direction
    virtual vec2i getNumTiles() const = 0;
    //! get number of pixels in x and y diretion
    virtual vec2i getNumPixels() const = 0;
    virtual size_t numMyTiles() const = 0;

    //! write given tile data into the frame buffer, sending to remove owner if required
    virtual void writeTile(size_t x0, size_t y0, size_t dxdy, uint32 *colorChannel, float *depthChannel) = 0;

    virtual void startNewFrame() = 0;
    virtual void closeCurrentFrame() = 0;

    vec2i numPixels,maxValidPixelID,tileSize;

    /*! function that will get called when the client is done (\see
      setTileWrittenCB */
    TileWrittenCB tileWrittenCBFunc;
    /*! additional 'data' parameter for the tileWrittenCBFunc (\see
      setTileWrittenCB) */
    void       *tileWrittenCBData;

  };

  //! create a new distributd frame buffer with given size and comm handle
  TiledFrameBuffer *createDistributedFrameBuffer(mpi::async::CommLayer *comm, 
                                                 //mpi::async::Group *comm, 
                                                 const vec2i &numPixels, 
                                                 size_t handle);

  template<int TILE_SIZE>
  struct DistributedFrameBuffer
    : public virtual TiledFrameBuffer
  {
    //! get number of pixels per tile, in x and y direction
    virtual vec2i getTileSize()  const { return vec2i(TILE_SIZE); };
    //! return number of tiles in x and y direction
    virtual vec2i getNumTiles()  const { return numTiles; };
    //! get number of pixels in x and y diretion
    virtual vec2i getNumPixels() const { return numPixels; }
    virtual size_t numMyTiles() const { return myTile.size(); };

    enum { 
      //! command tag that identifies a CommLayer::message as a write tile command
      COMMAND_WRITE_TILE = 13
    } COMMANDTAG;

    /*! raw tile data of TILE_SIZE x TILE_SIZE pixels (hardcoded for depth and color, for now) */
    struct TileData {
      uint32 color[TILE_SIZE][TILE_SIZE];
      float  depth[TILE_SIZE][TILE_SIZE];
    };

    //! message sent from one node's instance to another, to tell that instance to write that tile
    struct WriteTileMessage : public mpi::async::CommLayer::Message {
      vec2i    coords;
      TileData tile;
    };

    /*! one tile of the frame buffer. note that 'size' is the tile
      size used by the frame buffer, _NOT_ necessariy
      'end-begin'. 'color' and 'depth' arrays are always alloc'ed in tileSize pixels */
    struct Tile {
      DistributedFrameBuffer *fb;
      vec2i   begin;
      size_t  tileID,ownerID;
      TileData *data;

      Tile(DistributedFrameBuffer *fb, const vec2i &begin, size_t tileID, size_t ownerID, TileData *data);
    };

    //! constructor
    DistributedFrameBuffer(mpi::async::CommLayer *comm, 
                           const vec2i &numPixels, 
                           size_t myHandle);

    // ==================================================================
    // interface (as a tiled frame buffer) for renderers running on
    // the local node. 
    // ==================================================================

    //! write given tile data into the frame buffer, sending to remove owner if required
    virtual void writeTile(size_t x0, size_t y0, size_t dxdy, 
                           uint32 *colorChannel, float *depthChannel);

    //! return tile descriptor for given pixel coordinates. this tile may or may not belong to current instance
    inline Tile *getTileFor(size_t x, size_t y) const
    { return tile[tileIDof(x,y)]; }

    // ==================================================================
    // interface for the comm layer, to enable communication between
    // different instances of same object
    // ==================================================================

    //! handle incoming message from commlayer. it's the
    //! recipient's job to properly delete the message.
    virtual void incoming(mpi::async::CommLayer::Message *msg);

    // ==================================================================
    // internal helper functions
    // ==================================================================

    //! return the tile ID for given pair of coordinates. this tile may or may not belong to current instance
    inline size_t tileIDof(size_t x, size_t y) const
    { return (x/TILE_SIZE)+(y/TILE_SIZE)*numTiles.x; }
      
    virtual void startNewFrame();
    virtual void closeCurrentFrame() {
      mutex.lock();
      frameIsActive = false;
      frameIsDone   = true;
      mutex.unlock();
    };

    //! depth-composite additional 'source' info into existing 'target' tile
    static inline void depthComposite(TileData *target, TileData *source);
      
    vec2i numPixels,maxValidPixelID,numTiles;

    std::vector<Tile *> tile;    //!< list of all tiles
    std::vector<Tile *> myTile; //!< list of tiles belonging to this node. is a subset of 'tile' 

    Mutex mutex;
    //! set to true when the frame becomes 'active', and write tile
    //! messages can be consumed.
    bool frameIsActive, frameIsDone;

    size_t numTilesWritten;
    //! when messages arrive before a frame is active
    std::vector<WriteTileMessage *> delayedTile;
  };
    
}


