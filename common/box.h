// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

  /*! over over scalar type T and N dimensions */
  template<typename T, int N, int ALIGN=0>
  struct box_t {
    typedef T scalar_t;
    typedef typename ospcommon::vec_t<T,N,ALIGN> vec_t;
    
    inline box_t() {}
    inline box_t( EmptyTy ) : lower(pos_inf), upper(neg_inf) {}
    inline box_t(const box_t &o) : lower(o.lower), upper(o.upper) {}
    inline box_t(const vec_t &lower, const vec_t &upper) : lower(lower), upper(upper) {}

    inline vec_t size() const { return upper - lower; }
    inline void extend(const vec_t &v) { lower=min(lower,v), upper=max(upper,v); }
    inline void extend(const box_t &b) { lower=min(lower,b.lower), upper=max(upper,b.upper); }

    /*! returns the center of the box (not valid for empty boxes) */
    inline vec_t center() const { return 0.5f * (lower+upper); }
    inline bool empty() const { return anyLessThan(upper,lower); }

    vec_t lower, upper;
  };

  template<typename scalar_t>
  inline scalar_t area(const box_t<scalar_t,2> &b) { return b.size().product(); }

  /*! return the volume of the 3D box - undefined for empty boxes */
  template<typename scalar_t, int A>
  inline scalar_t volume(const box_t<scalar_t,3,A> &b) { return b.size().product(); }


  /*! compute the intersection of two boxes */
  template<typename T, int N, int A>
  inline box_t<T,N,A> intersectionOf(const box_t<T,N,A> &a, const box_t<T,N,A> &b)
  { return box_t<T,N,A>(max(a.lower,b.lower), min(a.upper,b.upper)); }

  template<typename T, int N>
  inline bool disjoint(const box_t<T,N> &a, const box_t<T,N> &b)
  { return anyLessThen(a.upper,b.lower) || anyLessThan(b.lower,a.upper); }


  /*! returns the center of the box (not valid for empty boxes) */
  template<typename T, int N, int A>
  inline vec_t<T,N,A> center(const box_t<T,N,A> &b)
  { return b.center(); }

  // -------------------------------------------------------
  // comparison operator
  // -------------------------------------------------------
  template<typename T, int N, int A>
  inline bool operator==(const box_t<T,N,A> &a, const box_t<T,N,A> &b)
  { return a.lower == b.lower && a.upper == b.upper; }
  template<typename T, int N, int A>
  inline bool operator!=(const box_t<T,N,A> &a, const box_t<T,N,A> &b)
  { return !(a == b); }

  // -------------------------------------------------------
  // output operator
  // -------------------------------------------------------
  template<typename T, int N, int A>
  std::ostream &operator<<(std::ostream &o, const box_t<T,N,A> &b)
  { o << "[" << b.lower <<":"<<b.upper<<"]"; return o; }


  typedef box_t<int32_t,2> box2i;
  typedef box_t<float,3> box3f;
  typedef box_t<float,3,1> box3fa;
  // typedef box_t<vec2i> box2i;
  
  // this is just a renaming - in some cases the code reads cleaner if
  // we're talking about 'regions' than about boxes
  typedef box2i region2i;
} // ::ospcommon
