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

#include "vec.h"

namespace ospcommon {

  template <int NDIMS>
  struct multidim_index_iterator;

  template <int NDIMS>
  struct multidim_index_sequence
  {
    static_assert(NDIMS == 2 || NDIMS == 3,
                  "ospcommon::multidim_index_sequence is currently limited to"
                  " only 2 or 3 dimensions. (NDIMS == 2 || NDIMS == 3)");

    multidim_index_sequence(const vec_t<int, NDIMS> &_dims);

    size_t flatten(const vec_t<int, NDIMS> &coords) const;

    vec_t<int, NDIMS> reshape(size_t i) const;

    vec_t<int, NDIMS> dimensions() const;

    size_t total_indices() const;

    multidim_index_iterator<NDIMS> begin() const;
    multidim_index_iterator<NDIMS> end()   const;

  private:

    vec_t<size_t, NDIMS> dims{0};
  };

  using index_sequence_2D = multidim_index_sequence<2>;
  using index_sequence_3D = multidim_index_sequence<3>;

  template <int NDIMS>
  struct multidim_index_iterator
  {
    multidim_index_iterator(const vec_t<int, NDIMS> &_dims) : dims(_dims) {}
    multidim_index_iterator(const vec_t<int, NDIMS> &_dims, size_t start)
      : multidim_index_iterator(_dims) { current_index = start; }

    // Traditional iterator interface methods //

    vec_t<int, NDIMS> operator*() const;

    multidim_index_iterator operator++();
    multidim_index_iterator &operator++(int);

    multidim_index_iterator operator--();
    multidim_index_iterator &operator--(int);

    multidim_index_iterator &operator+(const multidim_index_iterator &other);
    multidim_index_iterator &operator-(const multidim_index_iterator &other);

    multidim_index_iterator &operator+(int other);
    multidim_index_iterator &operator-(int other);

    bool operator==(const multidim_index_iterator &other) const;
    bool operator!=(const multidim_index_iterator &other) const;

    // Extra helper methods //

    void   jump_to(size_t index);
    size_t current() const;

  private:

    multidim_index_sequence<NDIMS> dims;
    size_t current_index{0};
  };

  // Inlined multidim_index_sequence definitions //////////////////////////////

  template <int NDIMS>
  inline multidim_index_sequence<NDIMS>::multidim_index_sequence(
    const vec_t<int, NDIMS> &_dims
  ) : dims(_dims)
  {
  }

  template <>
  inline size_t index_sequence_2D::flatten(const vec2i &coords) const
  {
    return coords.x + dims.x * coords.y;
  }

  template <>
  inline size_t index_sequence_3D::flatten(const vec3i &coords) const
  {
    return coords.x + dims.x * (coords.y + dims.y * coords.z);
  }

  template <>
  inline vec2i index_sequence_2D::reshape(size_t i) const
  {
    size_t y = i / dims.x;
    size_t x = i % dims.x;
    return vec2i(x, y);
  }

  template <>
  inline vec3i index_sequence_3D::reshape(size_t i) const
  {
    size_t z = i / (dims.x * dims.y);
    i -= (z * dims.x * dims.y);
    size_t y = i / dims.x;
    size_t x = i % dims.x;
    return vec3i(x, y, z);
  }

  template <int NDIMS>
  inline vec_t<int, NDIMS> multidim_index_sequence<NDIMS>::dimensions() const
  {
    return dims;
  }

  template <int NDIMS>
  inline size_t multidim_index_sequence<NDIMS>::total_indices() const
  {
    return dims.product();
  }

  template <int NDIMS>
  multidim_index_iterator<NDIMS> multidim_index_sequence<NDIMS>::begin() const
  {
    return multidim_index_iterator<NDIMS>(dims, 0);
  }

  template <int NDIMS>
  multidim_index_iterator<NDIMS> multidim_index_sequence<NDIMS>::end() const
  {
    return multidim_index_iterator<NDIMS>(dims, total_indices());
  }

  // Inlined multidim_index_iterator definitions //////////////////////////////

  template <int NDIMS>
  inline vec_t<int, NDIMS> multidim_index_iterator<NDIMS>::operator*() const
  {
    return dims.reshape(current_index);
  }

  template <int NDIMS>
  inline multidim_index_iterator<NDIMS>
  multidim_index_iterator<NDIMS>::operator++()
  {
    return multidim_index_iterator<NDIMS>(dims.dimensions(), ++current_index);
  }

  template <int NDIMS>
  inline multidim_index_iterator<NDIMS> &
  multidim_index_iterator<NDIMS>::operator++(int)
  {
    current_index++;
    return *this;
  }

  template <int NDIMS>
  inline multidim_index_iterator<NDIMS>
  multidim_index_iterator<NDIMS>::operator--()
  {
    return multidim_index_iterator<NDIMS>(dims.dimensions(), --current_index);
  }

  template <int NDIMS>
  inline multidim_index_iterator<NDIMS> &
  multidim_index_iterator<NDIMS>::operator--(int)
  {
    current_index--;
    return *this;
  }

  template <int NDIMS>
  inline multidim_index_iterator<NDIMS> &multidim_index_iterator<NDIMS>::
  operator+(const multidim_index_iterator &other)
  {
    current_index += other.current_index;
    return *this;
  }

  template <int NDIMS>
  inline multidim_index_iterator<NDIMS> &multidim_index_iterator<NDIMS>::
  operator-(const multidim_index_iterator &other)
  {
    current_index -= other.current_index;
    return *this;
  }

  template <int NDIMS>
  inline multidim_index_iterator<NDIMS> &
  multidim_index_iterator<NDIMS>::operator+(int offset)
  {
    current_index += offset;
    return *this;
  }

  template <int NDIMS>
  inline multidim_index_iterator<NDIMS> &
  multidim_index_iterator<NDIMS>::operator-(int offset)
  {
    current_index -= offset;
    return *this;
  }

  template <int NDIMS>
  inline bool multidim_index_iterator<NDIMS>::
  operator==(const multidim_index_iterator &other) const
  {
    return dims.dimensions() == other.dims.dimensions() &&
           current_index == other.current_index;
  }

  template <int NDIMS>
  inline bool multidim_index_iterator<NDIMS>::
  operator!=(const multidim_index_iterator &other) const
  {
    return !(*this == other);
  }

  template <int NDIMS>
  inline void multidim_index_iterator<NDIMS>::jump_to(size_t index)
  {
    current_index = index;
  }

  template <int NDIMS>
  inline size_t multidim_index_iterator<NDIMS>::current() const
  {
    return current_index;
  }

} // ::ospcommon
