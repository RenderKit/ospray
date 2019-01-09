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

#include "common/OSPCommon.h"

namespace ospray {

  static_assert(TILE_SIZE > 0 && (TILE_SIZE & (TILE_SIZE - 1)) == 0,
      "OSPRay config error: TILE_SIZE must be a positive power of two.");

  //! a tile of pixels used by any tile-based renderer
  /*! pixels in the tile are in a row-major TILE_SIZE x TILE_SIZE
      pattern. the 'region' specifies which part of the screen this
      tile belongs to: tile.lower is the lower-left coordinate of this
      tile (and a multiple of TILE_SIZE); the 'upper' value may be
      smaller than the upper-right edge of the "full" tile.

      note that a tile contains "all" of the values a renderer might
      want to use. not all renderers nor all frame buffers will use
      all those values; it's up to the renderer and frame buffer to
      agree on which fields will be set. Similarly, the frame buffer
      may actually use uchars, but the tile will always store
      floats. */
  struct OSPRAY_SDK_INTERFACE __aligned(64) Tile
  {
    region2i region; // screen region that this corresponds to
    vec2i    fbSize; // total frame buffer size, for the camera
    vec2f    rcp_fbSize;
    int32    generation;
    int32    children;
    int32    sortOrder;
    int32    accumID; //!< how often has been accumulated into this tile
    float    pad[4]; //!< padding to match the ISPC-side layout
    float    r[TILE_SIZE*TILE_SIZE];  // 'red' component
    float    g[TILE_SIZE*TILE_SIZE];  // 'green' component
    float    b[TILE_SIZE*TILE_SIZE];  // 'blue' component
    float    a[TILE_SIZE*TILE_SIZE];  // 'alpha' component
    float    z[TILE_SIZE*TILE_SIZE];  // 'depth' component
    float    nx[TILE_SIZE*TILE_SIZE]; // normal x
    float    ny[TILE_SIZE*TILE_SIZE]; // normal y
    float    nz[TILE_SIZE*TILE_SIZE]; // normal z
    float    ar[TILE_SIZE*TILE_SIZE]; // albedo red
    float    ag[TILE_SIZE*TILE_SIZE]; // albedo green
    float    ab[TILE_SIZE*TILE_SIZE]; // albedo blue

    Tile() = default;
    Tile(const vec2i &tile, const vec2i &fbsize, const int32 accumId)
      : fbSize(fbsize),
        rcp_fbSize(rcp(vec2f(fbsize))),
        generation(0),
        children(0),
        accumID(accumId)
    {
      region.lower = tile * TILE_SIZE;
      region.upper = ospcommon::min(region.lower + TILE_SIZE, fbsize);
    }
  };

} // ::ospray
