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
#include "DistributedFrameBuffer_TileTypes.h"
#include "DistributedFrameBuffer_ispc.h"

namespace ospray {

  using DFB = DistributedFrameBuffer;

  TileDesc::TileDesc(DFB *dfb, const vec2i &begin,
                     size_t tileID, size_t ownerID)
    : tileID(tileID), ownerID(ownerID), dfb(dfb), begin(begin)
  {}

  TileData::TileData(DFB *dfb, const vec2i &begin,
                     size_t tileID, size_t ownerID)
    : TileDesc(dfb,begin,tileID,ownerID)
  {}

  /*! called exactly once at the beginning of each frame */
  void AlphaBlendTile_simple::newFrame()
  {
    currentGeneration = 0;
    expectedInNextGeneration = 0;
    missingInCurrentGeneration = 1;

    assert(bufferedTile.empty());
  }

  void computeSortOrder(AlphaBlendTile_simple::BufferedTile *t)
  {
    float z = std::numeric_limits<float>::infinity();
    for (int iy=0;iy<t->tile.region.upper.y-t->tile.region.lower.y;iy++)
      for (int ix=0;ix<t->tile.region.upper.x-t->tile.region.lower.x;ix++)
        z = std::min(z,t->tile.z[ix+TILE_SIZE*iy]);
    t->sortOrder = z;
  }

  void TileData::accumulate(const ospray::Tile &tile)
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
      case OSP_FB_NONE:// NOTE(jda) - We accumulate here to enable PixelOps
                       //             working correctly here...this needs a
                       //             better solution!
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
  void AlphaBlendTile_simple::process(const ospray::Tile &tile)
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
  void WriteOnlyOnceTile::newFrame()
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
  void WriteOnlyOnceTile::process(const ospray::Tile &tile)
  {
    this->final.region = tile.region;
    this->final.fbSize = tile.fbSize;
    this->final.rcp_fbSize = tile.rcp_fbSize;
    accumulate(tile);
    dfb->tileIsCompleted(this);
  }

  void ZCompositeTile::newFrame()
  {
    numPartsComposited = 0;
  }

  void ZCompositeTile::process(const ospray::Tile &tile)
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

}// namespace ospray
