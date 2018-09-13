// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
    : dfb(dfb), begin(begin), tileID(tileID), ownerID(ownerID)
  {}

  TileData::TileData(DFB *dfb, const vec2i &begin,
                     size_t tileID, size_t ownerID)
    : TileDesc(dfb,begin,tileID,ownerID)
  {}

  AlphaBlendTile_simple::AlphaBlendTile_simple(DistributedFrameBuffer *dfb,
                                               const vec2i &begin,
                                               size_t tileID,
                                               size_t ownerID)
    : TileData(dfb,begin,tileID,ownerID)
  {}

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
    // accumulate, compute the final normalized colors, and compute the error,
    // but don't write the final colors into the color buffer yet because pixel
    // ops might modify them later
    // Note: also used for FB_NONE
    error = DFB_accumulateTile(
      (const ispc::VaryingTile*)&tile,
      (ispc::VaryingTile*)&final,
      (ispc::VaryingTile*)&accum,
      (ispc::VaryingTile*)&variance,
      dfb->hasAccumBuffer,
      dfb->hasVarianceBuffer);
    if (dfb->hasNormalBuffer || dfb->hasAlbedoBuffer)
      ispc::DFB_accumulateAuxTile((const ispc::VaryingTile*)&tile
          , (ispc::Tile*)&final
          , (ispc::VaryingTile*)&accum
          );
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
          for (uint32_t i = 0; i < bufferedTile.size(); i++) {
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
        for (uint32_t i = 0; i < bufferedTile.size(); i++) {
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

  void WriteMultipleTile::newFrame()
  {
    maxAccumID = 0;
    instances = dfb->tileInstances[tileID];
    writeOnceTile = instances <= 1;
    tileBuffered = false;
  }

  void WriteMultipleTile::process(const ospray::Tile &tile)
  {
    if (writeOnceTile) {
      final.region = tile.region;
      final.fbSize = tile.fbSize;
      final.rcp_fbSize = tile.rcp_fbSize;
      accumulate(tile);
      dfb->tileIsCompleted(this);
      return;
    }

    bool done = false;

    if (tile.accumID == 0) {
      final.region = tile.region;
      final.fbSize = tile.fbSize;
      final.rcp_fbSize = tile.rcp_fbSize;

      const auto bytes = tile.region.size().y * (TILE_SIZE * sizeof(float));
      memcpy(accum.z, tile.z, bytes);
      memcpy(final.z, tile.z, bytes);
    }

    {
      SCOPED_LOCK(mutex);
      maxAccumID = std::max(maxAccumID, tile.accumID);
      if (!tileBuffered && (tile.accumID & 1) == 0) {
        memcpy(&bufferedTile, &tile, sizeof(ospray::Tile));
        tileBuffered = true;
      } else {
        ispc::DFB_accumulateTileSimple(
          (const ispc::VaryingTile*)&tile,
          (ispc::VaryingTile*)&accum,
          (ispc::VaryingTile*)&variance);
        if (dfb->hasNormalBuffer || dfb->hasAlbedoBuffer)
          ispc::DFB_accumulateAuxTile((const ispc::VaryingTile*)&tile
              , (ispc::Tile*)&final
              , (ispc::VaryingTile*)&accum
              );
      }
      done = --instances == 0;
    }

    if (done) {
      auto sz = tile.region.size();
      if ((maxAccumID & 1) == 0) {
        // if maxAccumID is even, variance buffer is one accumulated tile
        // short, which leads to vast over-estimation of variance; thus
        // estimate variance now, when accum buffer is also one (the buffered)
        // tile short
        const float prevErr = DFB_computeErrorForTile(
          (ispc::vec2i&)sz,
          (ispc::VaryingTile*)&accum,
          (ispc::VaryingTile*)&variance,
          maxAccumID - 1);

        // use maxAccumID for correct normalization
        // this is OK, because both accumIDs are even
        bufferedTile.accumID = maxAccumID;
        accumulate(bufferedTile);
        error = prevErr;
      } else {
        // correct normalization is with maxAccumID, which is odd here
        bufferedTile.accumID = maxAccumID;
        // but original bufferedTile.accumID is always even and thus won't be
        // accumulated into variance buffer
        DFB_accumulateTile((const ispc::VaryingTile*)&bufferedTile
            , (ispc::VaryingTile*)&final
            , (ispc::VaryingTile*)&accum
            , (ispc::VaryingTile*)&variance
            , dfb->hasAccumBuffer
            , false // disable accumulation of variance
            );
        if (dfb->hasNormalBuffer || dfb->hasAlbedoBuffer)
          ispc::DFB_accumulateAuxTile((const ispc::VaryingTile*)&bufferedTile
              , (ispc::Tile*)&final
              , (ispc::VaryingTile*)&accum
              );
        // but still need to update the error
        error = DFB_computeErrorForTile((ispc::vec2i&)sz
            , (ispc::VaryingTile*)&accum
            , (ispc::VaryingTile*)&variance
            , maxAccumID
            );
      }

      dfb->tileIsCompleted(this);
    }
  }

  ZCompositeTile::ZCompositeTile(DistributedFrameBuffer *dfb,
                                 const vec2i &begin,
                                 size_t tileID,
                                 size_t ownerID,
                                 size_t numWorkers)
    : TileData(dfb,begin,tileID,ownerID),
      numWorkers(numWorkers)
  {}

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
        memcpy(&compositedTileData, &tile, sizeof(tile));
      else
        ispc::DFB_zComposite((const ispc::VaryingTile*)&tile,
                             (ispc::VaryingTile*)&this->compositedTileData);

      done = (++numPartsComposited == numWorkers);
    }

    if (done) {
      accumulate(this->compositedTileData);
      dfb->tileIsCompleted(this);
    }
  }

}// namespace ospray
