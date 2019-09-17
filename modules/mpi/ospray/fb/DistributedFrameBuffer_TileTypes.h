// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

  struct DistributedFrameBuffer;

  // -------------------------------------------------------
  /*! keeps the book-keeping of one tile of the frame buffer. note
    that 'size' is the tile size used by the frame buffer, _NOT_
    necessarily 'end-begin'. 'color' and 'depth' arrays are always
    alloc'ed in TILE_SIZE pixels */
  struct TileDesc
  {
    TileDesc(const vec2i &begin,
             size_t tileID,
             size_t ownerID);

    virtual ~TileDesc() = default;

    /*! returns whether this tile is one of this particular
        node's tiles */
    virtual bool mine() const { return false; }

    vec2i   begin;
    size_t  tileID, ownerID;
  };

  // -------------------------------------------------------
  /*! base class for a dfb tile. the only thing that all tiles have
      in common is depth, and RGBA-float accumulated data. Note we
      do not have a RGBA-I8 color field, because typically that'll
      be done by the postop and send-to-master op, and not stored in
      the DFB tile itself */
  struct TileData : public TileDesc
  {
    TileData(DistributedFrameBuffer *dfb,
             const vec2i &begin,
             size_t tileID,
             size_t ownerID);

    virtual ~TileData() override = default;

    /*! called exactly once at the beginning of each frame */
    virtual void newFrame() = 0;

    /*! returns whether this tile is one of this particular
        node's tiles */
    bool mine() const override { return true; }

    /*! called exactly once for each ospray::Tile that needs to get
        written into / composited into this dfb tile */
    virtual void process(const ospray::Tile &tile) = 0;

    // WILL debugging
    virtual bool isComplete() const { return false; }

    void accumulate(const ospray::Tile &tile);

    DistributedFrameBuffer *dfb;
    float error; // estimated variance of this tile
    // TODO: dynamically allocate to save memory when no ACCUM or VARIANCE
    // even more TODO: Tile contains much more data (e.g. AUX), but using only
    // the color buffer here ==> much wasted memory
    ospray::Tile __aligned(64) accum; // also hold accumulated normal&albedo
    ospray::Tile __aligned(64) variance;
    /* iw: TODO - have to change this. right now, to be able to give
       the 'postaccum' pixel op a readily normalized tile we have to
       create a local copy (the tile stores only the accum value,
       and we cannot change this) */
    // also holds normalized normal&albedo in AOS format
    ospray::Tile  __aligned(64) final;

    //! the rbga32-converted colors
    // TODO: dynamically allocate to save memory when only uint32 / I8 colors
    vec4f __aligned(64) color[TILE_SIZE*TILE_SIZE];
  };


  // -------------------------------------------------------
  /*! specialized tile for plain sort-first rendering, but where the same tile
      region could be computed multiple times (with different accumId). */
  struct WriteMultipleTile : public TileData
  {
    WriteMultipleTile(DistributedFrameBuffer *dfb
        , const vec2i &begin
        , size_t tileID
        , size_t ownerID
        )
      : TileData(dfb, begin, tileID, ownerID)
        , writeOnceTile(true)
    {}

    void newFrame() override;

    // accumulate into ACCUM and VARIANCE, and for last tile read-out
    void process(const ospray::Tile &tile) override;

   private:
    int maxAccumID;
    size_t instances;
    bool writeOnceTile;
    // serialize when multiple instances of this tile arrive at the same time
    std::mutex mutex;
    // defer accumulation to get correct variance estimate
    ospray::Tile bufferedTile;
    bool tileBuffered;
  };

  // -------------------------------------------------------
  /*! specialized tile for doing Z-compositing. */
  struct ZCompositeTile : public TileData
  {
    ZCompositeTile(DistributedFrameBuffer *dfb, const vec2i &begin,
                   size_t tileID, size_t ownerID, size_t numWorkers);

    /*! called exactly once at the beginning of each frame */
    void newFrame() override;

    /*! called exactly once for each ospray::Tile that needs to get
        written into / composited into this dfb tile */
    void process(const ospray::Tile &tile) override;

    /*! number of input tiles that have been composited into this
        tile */
    size_t numPartsComposited;

    size_t numWorkers;

    /*! since we do not want to mess up the existing accumulatation
        buffer in the parent tile we temporarily composite into this
        buffer until all the composites have been done. */
    ospray::Tile compositedTileData;
    std::mutex mutex;
  };

  /*! specialized tile implementation that first buffers all
      ospray::Tile's until all input tiles are available, then sorts
      them by closest z component per tile, and only tthen does
      front-to-back compositing of those tiles */
  struct AlphaBlendTile : public TileData
  {
    AlphaBlendTile(DistributedFrameBuffer *dfb,
                   const vec2i &begin,
                   size_t tileID,
                   size_t ownerID);

    /*! called exactly once at the beginning of each frame */
    void newFrame() override;

    /*! called exactly once for each ospray::Tile that needs to get
        written into / composited into this dfb tile */
    void process(const ospray::Tile &tile) override;

    bool isComplete() const override { return missingInCurrentGeneration == 0; }

    struct BufferedTile
    {
      ospray::Tile tile;

      /*! determines order of this tile relative to other tiles.

        Tiles will get blended with the 'over' operator in
        increasing 'BufferedTile::sortOrder' value */
      float sortOrder;
    };

  private:
    std::vector<BufferedTile *> bufferedTile;
    int currentGeneration;
    int expectedInNextGeneration;
    int missingInCurrentGeneration;
    std::mutex mutex;

    void reportCompositingError(const vec2i &tile);
  };

} // namespace ospray
