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

#pragma once

#include "vec.h"

namespace ospcommon {

  template <int NDIMS>
  struct multidim_index_sequence
  {
    static_assert(NDIMS == 2 || NDIMS == 3,
                  "ospcommon::multidim_index_sequence is currently limited to"
                  " only 2 or 3 dimensions. (NDIMS == 2 || NDIMS == 3)");

    using vec_index_t = vec_t<int   , NDIMS>;
    using vec_store_t = vec_t<size_t, NDIMS>;

    multidim_index_sequence(const vec_index_t &_dims);

    size_t flatten(const vec_index_t &coords);

    vec_index_t reshape(size_t i);

    size_t total_indices();

    // TODO: iterators...

  private:

    vec_store_t dims{0};
  };

  using index_sequence_2D = multidim_index_sequence<2>;
  using index_sequence_3D = multidim_index_sequence<3>;

  // Inlined definitions //////////////////////////////////////////////////////

  template <int NDIMS>
  inline multidim_index_sequence<NDIMS>::multidim_index_sequence(
    const multidim_index_sequence<NDIMS>::vec_index_t &_dims
  ) : dims(_dims) {}

  template <>
  inline size_t index_sequence_2D::flatten(const vec2i &coords)
  {
    return coords.x + dims.x * coords.y;
  }

  template <>
  inline size_t index_sequence_3D::flatten(const vec3i &coords)
  {
    return coords.x + dims.x * (coords.y + dims.y * coords.z);
  }

  template <>
  inline index_sequence_2D::vec_index_t index_sequence_2D::reshape(size_t i)
  {
    size_t y = i / dims.x;
    size_t x = i % dims.x;
    return vec_index_t(x, y);
  }

  template <>
  inline index_sequence_3D::vec_index_t index_sequence_3D::reshape(size_t i)
  {
    size_t z = i / (dims.x * dims.y);
    i -= (z * dims.x * dims.y);
    size_t y = i / dims.x;
    size_t x = i % dims.x;
    return vec_index_t(x, y, z);
  }

  template <int NDIMS>
  inline size_t multidim_index_sequence<NDIMS>::total_indices()
  {
    return dims.product();
  }

} // ::ospcommon