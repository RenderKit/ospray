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

#include "../box.h"

/*! \file array3D/for_each Helper templates to do 3D iterations via
  lambda functions */

namespace ospcommon {
  namespace array3D {

    /*! compute - in 64 bit - the number of voxels in a vec3i */
    inline size_t longProduct(const vec3i &dims)
    {
      return dims.x * size_t(dims.y) * dims.z;
    }

    /*! compute - in 64 bit - the linear array index of vec3i index in
        a vec3i sized array */
    inline size_t longIndex(const vec3i &idx, const vec3i &dims)
    {
      return idx.x + size_t(dims.x) * (idx.y + size_t(dims.y) * idx.z);
    }

    /*! reverse mapping form a cell index to the cell coordinates, for
        a given volume size 'dims' */
    inline vec3i coordsOf(const size_t idx, const vec3i &dims)
    {
      return vec3i(idx % dims.x,
                   (idx / dims.x) % dims.y,
                   (idx / dims.x) / dims.y);
    }

    /*! iterate through all indices in [lower,upper), EXCLUSING the
        'upper' value */
    template <typename Functor>
    inline void for_each(const vec3i &lower,
                         const vec3i &upper,
                         Functor &&functor)
    {
      for (int iz = lower.z; iz < upper.z; iz++)
        for (int iy = lower.y; iy < upper.y; iy++)
          for (int ix = lower.x; ix < upper.x; ix++)
            functor(vec3i(ix, iy, iz));
    }

    /*! a template that calls the given functor (typically a lambda) for
      every vec3i(ix,iy,iz) with 0<=ix<xize.x, 0<=iy<size.y, and
      0<=iz<size.z)

      Example:

      array3D::for_each(volume->size(),[&](const vec3i &idx){
        doSomeThing(volume,index);
      });
    */
    template <typename Functor>
    inline void for_each(const vec3i &size, Functor &&functor)
    {
      for_each({0, 0, 0}, size, std::forward<Functor>(functor));
    }

    /*! iterate through all indices in [lower,upper), EXCLUSING the
        'upper' value */
    template <typename Functor>
    inline void for_each(const box3i &coords, Functor &&functor)
    {
      for_each(coords.lower, coords.upper, std::forward<Functor>(functor));
    }

  }  // ::ospcommon::array3D
}  // ::ospcommon
