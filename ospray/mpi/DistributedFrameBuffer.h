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
#include "ospray/fb/tileSize.h"
#include "ospray/fb/LocalFB.h"

namespace ospray {
  using std::cout;
  using std::endl;

  struct DistributedFrameBuffer
    : public mpi::async::CommLayer::Object,
      public virtual FrameBuffer
  {
    //! get number of pixels per tile, in x and y direction
    virtual vec2i getTileSize()  const { return vec2i(TILE_SIZE); };
    //! return number of tiles in x and y direction
    virtual vec2i getNumTiles()  const { return numTiles; };
    //! get number of pixels in x and y diretion
    virtual vec2i getNumPixels() const { return numPixels; }
    virtual size_t numMyTiles() const { return myTile.size(); };

    /*! color buffer and depth buffer on master */
    void  *colorBufferOnMaster;
    float *depthBufferOnMaster;
    
    enum { 
      //! command tag that identifies a CommLayer::message as a write tile command
      WORKER_WRITE_TILE = 13,
      MASTER_WRITE_TILE
    } COMMANDTAG;

    /*! raw tile data of TILE_SIZE x TILE_SIZE pixels (hardcoded for depth and color, for now) */
    struct DFBTileData {
      float  accum_r[TILE_SIZE][TILE_SIZE];
      float  accum_g[TILE_SIZE][TILE_SIZE];
      float  accum_b[TILE_SIZE][TILE_SIZE];
      float  accum_a[TILE_SIZE][TILE_SIZE];
      uint32 color[TILE_SIZE][TILE_SIZE];
      float  depth[TILE_SIZE][TILE_SIZE];
    };

    //! message sent from one node's instance to another, to tell that instance to write that tile
    struct WriteTileMessage : public mpi::async::CommLayer::Message {
      // TODO: add compression of pixels during transmission
      vec2i coords;
      float r[TILE_SIZE][TILE_SIZE];
      float g[TILE_SIZE][TILE_SIZE];
      float b[TILE_SIZE][TILE_SIZE];
      float a[TILE_SIZE][TILE_SIZE];
    };

    // /*! raw tile data of TILE_SIZE x TILE_SIZE pixels (hardcoded for depth and color, for now) */
    // struct DFBTileData : public ospray::DFBTile {
    //   // uint32 color[TILE_SIZE][TILE_SIZE];
    //   // float  depth[TILE_SIZE][TILE_SIZE];
    // };

    // //! message sent from one node's instance to another, to tell that instance to write that tile
    // struct WriteDFBTileMessage : public mpi::async::CommLayer::Message {
    //   vec2i    coords;
    //   DFBTileData tile;
    // };

    /*! one tile of the frame buffer. note that 'size' is the tile
      size used by the frame buffer, _NOT_ necessariy
      'end-begin'. 'color' and 'depth' arrays are always alloc'ed in TILE_SIZE pixels */
    struct DFBTile {
      DistributedFrameBuffer *fb;
      vec2i   begin;
      size_t  tileID,ownerID;
      DFBTileData *data;

      DFBTile(DistributedFrameBuffer *fb, 
           const vec2i &begin, 
           size_t tileID, 
           size_t ownerID, 
           DFBTileData *data);
    };

    /*! local frame buffer on the master used for storing the final
        tiles. will be null on all workers, and _may_ be null on the
        master if the master does not have a color buffer */
    Ref<LocalFrameBuffer> localFBonMaster;

    //! constructor
    DistributedFrameBuffer(mpi::async::CommLayer *comm, 
                           const vec2i &numPixels, 
                           size_t myHandle,
                           ColorBufferFormat colorBufferFormat,
                           bool hasDepthBuffer,
                           bool hasAccumBuffer);

    virtual const void *mapDepthBuffer() { NOTIMPLEMENTED; }
    virtual const void *mapColorBuffer() { NOTIMPLEMENTED; }
    virtual void unmap(const void *mappedMem) { NOTIMPLEMENTED; }
    virtual void setTile(ospray::Tile &tile)  { NOTIMPLEMENTED; }

    /*! \brief clear (the specified channels of) this frame buffer 

      \details for the *distributed* frame buffer, we assume that
      *all* nodes get this command, and that each instance therefore
      can clear only its own tiles without having to tell any other
      node about it
     */
    virtual void clear(const uint32 fbChannelFlags);

    //! write given tile data into the frame buffer, sending to remove owner if required
    virtual void writeTile(ospray::Tile &tile);
    // virtual void writeTile(size_t x0, size_t y0, size_t dxdy, 
    //                        uint32 *colorChannel, float *depthChannel);

    //! return tile descriptor for given pixel coordinates. this tile
    //! may or may not belong to current instance
    inline DFBTile *getTileFor(size_t x, size_t y) const
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

    // //! depth-composite additional 'source' info into existing 'target' tile
    // static inline void depthComposite(DFBTileData *target, DFBTileData *source);
      
    vec2i numPixels,maxValidPixelID,numTiles;

    std::vector<DFBTile *> tile;    //!< list of all tiles
    std::vector<DFBTile *> myTile; //!< list of tiles belonging to this node. is a subset of 'tile' 

    Mutex mutex;
    //! set to true when the frame becomes 'active', and write tile
    //! messages can be consumed.
    bool frameIsActive, frameIsDone;

    size_t numTilesWritten;
    //! when messages arrive before a frame is active
    std::vector<WriteTileMessage *> delayedTile;
  };
    
}


