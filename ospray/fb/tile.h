#pragma once

#include "ospray/common/ospcommon.h"
#include "tilesize.h"

namespace ospray {

  typedef embree::BBox<embree::Vec2i> region2i;

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
    uint32   format; /*! or'ed-together bits describing what format
                       those pixels have */
  };
}
