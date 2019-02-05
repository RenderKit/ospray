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
#include "range.h"

namespace ospcommon {

  // box declaration //////////////////////////////////////////////////////////

  template <typename T, int N, bool ALIGN = false>
  using box_t = range_t<vec_t<T, N, ALIGN>>;

  // box free functions ///////////////////////////////////////////////////////

  template <typename scalar_t>
  inline scalar_t area(const box_t<scalar_t, 2> &b)
  {
    return b.size().product();
  }

  template <typename scalar_t, bool A>
  inline scalar_t area(const box_t<scalar_t, 3, A> &b)
  {
    const auto size = b.size();
    return 2.f * (size.x * size.y + size.x * size.z + size.y * size.z);
  }

  /*! return the volume of the 3D box - undefined for empty boxes */
  template <typename scalar_t, bool A>
  inline scalar_t volume(const box_t<scalar_t, 3, A> &b)
  {
    return b.size().product();
  }

  /*! computes whether two boxes are either touching OR overlapping;
      ie, the case where boxes just barely touch side-by side (even if
      they do not have any actual overlapping _volume_!) then this is
      still true */
  template <typename scalar_t, bool A>
  inline bool touchingOrOverlapping(const box_t<scalar_t, 3, A> &a,
                                    const box_t<scalar_t, 3, A> &b)
  {
    if (a.lower.x > b.upper.x)
      return false;
    if (a.lower.y > b.upper.y)
      return false;
    if (a.lower.z > b.upper.z)
      return false;

    if (b.lower.x > a.upper.x)
      return false;
    if (b.lower.y > a.upper.y)
      return false;
    if (b.lower.z > a.upper.z)
      return false;

    return true;
  }

  template <typename scalar_t, bool A>
  inline bool touchingOrOverlapping(const box_t<scalar_t, 2, A> &a,
                                    const box_t<scalar_t, 2, A> &b)
  {
    if (a.lower.x > b.upper.x)
      return false;
    if (a.lower.y > b.upper.y)
      return false;

    if (b.lower.x > a.upper.x)
      return false;
    if (b.lower.y > a.upper.y)
      return false;

    return true;
  }

  /*! compute the intersection of two boxes */
  template <typename T, int N, bool A>
  inline box_t<T, N, A> intersectionOf(const box_t<T, N, A> &a,
                                       const box_t<T, N, A> &b)
  {
    return box_t<T, N, A>(max(a.lower, b.lower), min(a.upper, b.upper));
  }

  template <typename T, int N>
  inline bool disjoint(const box_t<T, N> &a, const box_t<T, N> &b)
  {
    return anyLessThan(a.upper, b.lower) || anyLessThan(b.lower, a.upper);
  }

  /*! returns the center of the box (not valid for empty boxes) */
  template <typename T, int N, bool A>
  inline vec_t<T, N, A> center(const box_t<T, N, A> &b)
  {
    return b.center();
  }

  // -------------------------------------------------------
  // comparison operator
  // -------------------------------------------------------
  template <typename T, int N, bool A>
  inline bool operator==(const box_t<T, N, A> &a, const box_t<T, N, A> &b)
  {
    return a.lower == b.lower && a.upper == b.upper;
  }

  template <typename T, int N, bool A>
  inline bool operator!=(const box_t<T, N, A> &a, const box_t<T, N, A> &b)
  {
    return !(a == b);
  }

  // -------------------------------------------------------
  // output operator
  // -------------------------------------------------------
  template <typename T, int N, bool A>
  inline std::ostream &operator<<(std::ostream &o, const box_t<T, N, A> &b)
  {
    o << "[" << b.lower << ":" << b.upper << "]";
    return o;
  }

  using box2i  = box_t<int32_t, 2>;
  using box3i  = box_t<int32_t, 3>;
  using box1f  = box_t<float, 1>;
  using box2f  = box_t<float, 2>;
  using box3f  = box_t<float, 3>;
  using box4f  = box_t<float, 4>;
  using box3fa = box_t<float, 3, 1>;

  // this is just a renaming - in some cases the code reads cleaner if
  // we're talking about 'regions' than about boxes
  using region2i = box2i;

} // ::ospcommon
