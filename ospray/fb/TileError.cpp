// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "TileError.h"
#include <vector>

namespace ospray {

  TileError::TileError(const vec2i &_numTiles)
    : numTiles(_numTiles)
    , tiles(_numTiles.x * _numTiles.y)
  {
    if (tiles > 0)
      tileErrorBuffer = alignedMalloc<float>(tiles);
    else
      tileErrorBuffer = nullptr;

    // maximum number of regions: all regions are of size 3 are split in half
    errorRegion.reserve(divRoundUp(tiles * 2, 3));
  }

  TileError::~TileError()
  {
    alignedFree(tileErrorBuffer);
  }

  void TileError::clear()
  {
    for (int i = 0; i < tiles; i++)
      tileErrorBuffer[i] = inf;

    errorRegion.clear();
    // initially create one region covering the complete image
    errorRegion.push_back(box2i(vec2i(0), numTiles));
  }

  float TileError::operator[](const vec2i &tile) const
  {
    if (tiles <= 0)
      return inf;

    return tileErrorBuffer[tile.y * numTiles.x + tile.x];
  }

  void TileError::update(const vec2i &tile, const float err)
  {
    if (tiles > 0)
      tileErrorBuffer[tile.y * numTiles.x + tile.x] = err;
  }

  float TileError::refine(const float errorThreshold)
  {
    if (tiles <= 0)
      return inf;

    // process regions first, but don't process newly split regions again
    int regions = errorThreshold > 0.f ? errorRegion.size() : 0;
    for (int i = 0; i < regions; i++) {
      box2i& region = errorRegion[i];
      float err = 0.f;
      float maxErr = 0.f;
      for (int y = region.lower.y; y < region.upper.y; y++)
        for (int x = region.lower.x; x < region.upper.x; x++) {
          int idx = y * numTiles.x + x;
          err += tileErrorBuffer[idx];
          maxErr = std::max(maxErr, tileErrorBuffer[idx]);
        }
      if (maxErr > errorThreshold) {
        // set all tiles of this region to >errorThreshold to enforce their
        // refinement as a group
        const float minErr = nextafter(errorThreshold, inf);
        for (int y = region.lower.y; y < region.upper.y; y++)
          for (int x = region.lower.x; x < region.upper.x; x++) {
            int idx = y * numTiles.x + x;
            tileErrorBuffer[idx] = std::max(tileErrorBuffer[idx], minErr);
          }
      }
      const vec2i size = region.size();
      const int area = reduce_mul(size);
      err /= area; // == avg
      if (err <= 4.f*errorThreshold) { // split region?
        // if would just contain single tile after split or wholly done: remove
        if (area <= 2 || maxErr <= errorThreshold) {
          regions--;
          errorRegion[i] = errorRegion[regions];
          errorRegion[regions] = errorRegion.back();
          errorRegion.pop_back();
          i--;
          continue;
        }
        const vec2i split = region.lower + size / 2; // TODO: find split with
                                                     //       equal variance
        errorRegion.push_back(region); // region ref might become invalid
        if (size.x > size.y) {
          errorRegion[i].upper.x = split.x;
          errorRegion.back().lower.x = split.x;
        } else {
          errorRegion[i].upper.y = split.y;
          errorRegion.back().lower.y = split.y;
        }
      }
    }

    return *std::max_element(tileErrorBuffer, tileErrorBuffer + tiles);
  }
} // ::ospray
