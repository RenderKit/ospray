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

// amr base
#include "ospcommon/array3D/Array3D.h"
// ospray
#include "common/Data.h"

namespace ospray {
  namespace amr {

    /*! this structure defines only the format of the INPUT of amr
        data - ie, what we get from the scene graph or applicatoin */
    struct AMRData
    {
      AMRData(const Data &brickInfoData, const Data &brickDataData);

      /*! this is how an app _specifies_ a brick (or better, the array
        of bricks); the brick data is specified through a separate
        array of data buffers (one data buffer per brick) */
      struct BrickInfo
      {
        /*! bounding box of integer coordinates of cells. note that
          this EXCLUDES the width of the rightmost cell: ie, a 4^3
          box at root level pos (0,0,0) would have a _box_ of
          [(0,0,0)-(3,3,3)] (because 3,3,3 is the highest valid
          coordinate in this box!), while its bounds would be
          [(0,0,0)-(4,4,4)]. Make sure to NOT use box.size() for the
          grid dimensions, since this will always be one lower than
          the dims of the grid.... */
        box3i box;
        //! level this brick is at
        int   level;
        // width of each cell in this level
        float cellWidth;

        inline box3f worldBounds() const
        {
          return box3f(vec3f(box.lower)*cellWidth,
                       vec3f(box.upper+vec3i(1))*cellWidth);
        }
      };

      struct Brick : public BrickInfo
      {
        /*! actual constructor from a brick info and data pointer */
        /*! initialize from given data */
        Brick(const BrickInfo &info, const float *data);

        /* world bounds, including entire cells, and including
           level-specific cell width. ie, at root level cell width of
           1, a 4^3 root brick at (0,0,0) would have bounds
           [(0,0,0)-(4,4,4)] (as opposed to the 'box' value, see
           above!) */
        box3f worldBounds;

        //! pointer to the actual data values stored in this brick
        const float *value{nullptr};
        //! dimensions of this box's data
        vec3i dims;
        //! scale factor from grid space to world space (ie,1.f/cellWidth)
        float gridToWorldScale;
        //! rcp(bounds.upper-bounds.lower);
        vec3f worldToGridScale;
        //! dimensions, in float
        vec3f f_dims;
      };

      //! our own, internal representation of a brick
      std::vector<Brick> brick;

      /*! compute world-space bounding box (lot in _logical_ space,
          but in _absolute_ space, with proper cell width as specified
          in each level */
      inline box3f worldBounds() const
      {
        box3f worldBounds = empty;
        for (const auto &b : brick)
          worldBounds.extend(b.worldBounds);
        return worldBounds;
      }
    };

  } // ::ospray::amr
} // ::ospray
