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

#pragma once

#include "fb/Tile.h"

#include <vector>

namespace ospray {

  class DistributedFrameBuffer;

  // -------------------------------------------------------
  /*! keeps the book-keeping of one tile of the frame buffer. note
    that 'size' is the tile size used by the frame buffer, _NOT_
    necessariy 'end-begin'. 'color' and 'depth' arrays are always
    alloc'ed in TILE_SIZE pixels */
  struct TileDesc {
    ALIGNED_STRUCT

    /*! constructor */
    TileDesc(DistributedFrameBuffer *dfb,
             const vec2i &begin,
             size_t tileID,
             size_t ownerID);

    /*! returns whether this tile is one of this particular
        node's tiles */
    virtual bool mine() const { return false; }

    DistributedFrameBuffer *dfb;
    vec2i   begin;
    size_t  tileID,ownerID;
  };

  // -------------------------------------------------------
  /*! base class for a dfb tile. the only thing that all tiles have
      in common is depth, and RGBA-float accumulated data. Note we
      do not have a RGBA-I8 color field, because typically that'll
      be done by the postop and send-to-master op, and not stored in
      the DFB tile itself */
  struct TileData : public TileDesc {
    TileData(DistributedFrameBuffer *dfb,
             const vec2i &begin,
             size_t tileID,
             size_t ownerID);

    /*! called exactly once at the beginning of each frame */
    virtual void newFrame() = 0;

    /*! returns whether this tile is one of this particular
        node's tiles */
    bool mine() const override { return true; }

    /*! called exactly once for each ospray::Tile that needs to get
        written into / composited into this dfb tile */
    virtual void process(const ospray::Tile &tile) = 0;

    void accumulate(const ospray::Tile &tile);

    float error; // estimated variance of this tile
    // TODO: dynamically allocate to save memory when no ACCUM or VARIANCE
    ospray::Tile __aligned(64) accum;
    ospray::Tile __aligned(64) variance;
    /* iw: TODO - have to change this. right now, to be able to give
       the 'postaccum' pixel op a readily normalized tile we have to
       create a local copy (the tile stores only the accum value,
       and we cannot change this) */
    ospray::Tile  __aligned(64) final;

    //! the rbga32-converted colors
    // TODO: dynamically allocate to save memory when only uint32 / I8 colors
    vec4f __aligned(64) color[TILE_SIZE*TILE_SIZE];
  };

  // -------------------------------------------------------
  /*! specialized tile for plain sort-first rendering, where each
      tile is written only exactly once. */
  struct WriteOnlyOnceTile : public TileData {

    /*! constructor */
    WriteOnlyOnceTile(DistributedFrameBuffer *dfb,
                      const vec2i &begin,
                      size_t tileID,
                      size_t ownerID)
      : TileData(dfb,begin,tileID,ownerID)
    {}

    /*! called exactly once at the beginning of each frame */
    void newFrame() override;

    /*! called exactly once for each ospray::Tile that needs to get
      written into / composited into this dfb tile.

      for a write-once tile, we expect this to be called exactly
      once per tile, so there's not a lot to do in here than
      accumulating the tile data and telling the parent that we're
      done.
    */
    void process(const ospray::Tile &tile) override;
  };

  // -------------------------------------------------------
  /*! specialized tile for doing Z-compositing. this does not have
      additional data, but a different write op. */
  struct ZCompositeTile : public TileData {

    /*! constructor */
    ZCompositeTile(DistributedFrameBuffer *dfb,
                   const vec2i &begin,
                   size_t tileID,
                   size_t ownerID)
      : TileData(dfb,begin,tileID,ownerID)
    {}

    /*! called exactly once at the beginning of each frame */
    void newFrame() override;

    /*! called exactly once for each ospray::Tile that needs to get
        written into / composited into this dfb tile */
    void process(const ospray::Tile &tile) override;

    /*! number of input tiles that have been composited into this
        tile */
    size_t numPartsComposited;

    /*! since we do not want to mess up the existing accumulatation
        buffer in the parent tile we temporarily composite into this
        buffer until all the composites have been done. */
    ospray::Tile compositedTileData;
    Mutex mutex;
  };

  /*! specialized tile implementation that first buffers all
      ospray::Tile's until all input tiles are available, then sorts
      them by closest z component per tile, and only tthen does
      front-to-back compositing of those tiles */
  struct AlphaBlendTile_simple : public TileData {

    /*! constructor */
    AlphaBlendTile_simple(DistributedFrameBuffer *dfb,
                   const vec2i &begin,
                   size_t tileID,
                   size_t ownerID)
      : TileData(dfb,begin,tileID,ownerID)
    {}

    /*! called exactly once at the beginning of each frame */
    void newFrame() override;

    /*! called exactly once for each ospray::Tile that needs to get
        written into / composited into this dfb tile */
    void process(const ospray::Tile &tile) override;

    struct BufferedTile {
      ospray::Tile tile;

      /*! determines order of this tile relative to other tiles.

        Tiles will get blended with the 'over' operator in
        increasing 'BufferedTile::sortOrder' value */
      float sortOrder;
    };
    std::vector<BufferedTile *> bufferedTile;
    int currentGeneration;
    int expectedInNextGeneration;
    int missingInCurrentGeneration;
    Mutex mutex;
  };

} // namespace ospray
