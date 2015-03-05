// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "ospray/common/OSPCommon.h"
#include "tileSize.h"

namespace ospray {

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
  struct __aligned(64) Tile {
    // 'red' component; in float.
    float r[TILE_SIZE*TILE_SIZE];
    // 'green' component; in float.
    float g[TILE_SIZE*TILE_SIZE];
    // 'blue' component; in float.
    float b[TILE_SIZE*TILE_SIZE];
    // 'alpha' component; in float.
    float a[TILE_SIZE*TILE_SIZE];
    // 'depth' component; in float.
    float z[TILE_SIZE*TILE_SIZE];
    region2i region; /*!< screen region that this corresponds to */
    vec2i    fbSize; /*!< total frame buffer size, for the camera */
    vec2f    rcp_fbSize;
  };

} // ::ospray
