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

#include "../common/common.h"

namespace ospcommon {
  
  template<typename T>
  struct vec2_t {
    typedef T scalar_t;

    inline vec2_t() {};
    inline vec2_t(const vec2_t &o) : x(o.x), y(o.y) {}
    inline vec2_t(scalar_t s) : x(s), y(s) {};
    inline vec2_t(scalar_t x, scalar_t y) : x(x), y(y) {};

    /*! return result of reduce_add() across all components */
    inline scalar_t sum() const { return x+y; }
    /*! return result of reduce_mul() across all components */
    inline scalar_t product() const { return x*y; }

    T x, y;
  };

  template<typename T>
  struct vec3_t {
    typedef T scalar_t;

    inline vec3_t() {};
    inline vec3_t(scalar_t s) : x(s), y(s) {};
    inline vec3_t(scalar_t x, scalar_t y, scalar_t z) : x(x), y(y), z(z) {};

    /*! return result of reduce_add() across all components */
    inline scalar_t sum() const { return x+y+z; }
    /*! return result of reduce_mul() across all components */
    inline scalar_t product() const { return x*y*z; }

    T x, y, z;
  };

  template<typename T>
  struct vec4_t {
    typedef T scalar_t;

    inline vec4_t() {};
    inline vec4_t(scalar_t s) : x(s), y(s) {};
    inline vec4_t(scalar_t x, scalar_t y, scalar_t z, scalar_t w) : x(x), y(y), z(z), w(w) {};

    /*! return result of reduce_add() across all components */
    inline scalar_t sum() const { return x+y+z+w; }
    /*! return result of reduce_mul() across all components */
    inline scalar_t product() const { return x*y*z*w; }

    T x, y, z, w;
  };



  // -------------------------------------------------------
  // all vec2 variants
  // -------------------------------------------------------
  typedef vec2_t<uint32_t> vec2ui;
  typedef vec2_t<int32_t>  vec2i;
  typedef vec2_t<float>    vec2f;
  typedef vec2_t<double>   vec2d;

  // -------------------------------------------------------
  // all vec3 variants
  // -------------------------------------------------------
  typedef vec3_t<uint32_t> vec3ui;
  typedef vec3_t<int32_t>  vec3i;
  typedef vec3_t<float>    vec3f;
  typedef vec3_t<double>   vec3d;

  // -------------------------------------------------------
  // binary operators
  // -------------------------------------------------------
#define define_operator(operator,op)		\
  template<typename T>                                                \
  inline vec2_t<T> operator(const vec2_t<T> &a, const vec2_t<T> &b)   \
  { return vec2_t<T>(a.x op b.x, a.y op b.y); }                       \
                                                                      \
  template<typename T>                                                \
  inline vec3_t<T> operator(const vec3_t<T> &a, const vec3_t<T> &b)   \
  { return vec3_t<T>(a.x op b.x, a.y op b.y, a.z op b.z); }           \
                                                                      \
  template<typename T>                                                \
  inline vec4_t<T> operator(const vec4_t<T> &a, const vec4_t<T> &b)     \
  { return vec4_t<T>(a.x op b.x, a.y op b.y, a.z op b.z, a.w op b.w); } \

  define_operator(operator*,*);
  define_operator(operator/,/);
  define_operator(operator-,-);
  define_operator(operator+,+);
#undef define_operator

  // "inherit" std::min/max/etc for basic types
  using std::min;
  using std::max;

  // -------------------------------------------------------
  // binary functors
  // -------------------------------------------------------
#define define_functor(f)                                             \
  template<typename T>                                                \
  inline vec2_t<T> f(const vec2_t<T> &a, const vec2_t<T> &b)          \
  { return vec2_t<T>(f(a.x,b.x),f(a.y,b.y)); }                        \
                                                                      \
  template<typename T>                                                \
  inline vec3_t<T> f(const vec3_t<T> &a, const vec3_t<T> &b)          \
  { return vec3_t<T>(f(a.x,b.x),f(a.y,b.y),f(a.z,b.z)); }             \
                                                                      \
  template<typename T>                                                  \
  inline vec4_t<T> f(const vec4_t<T> &a, const vec4_t<T> &b)            \
  { return vec4_t<T>(f(a.x,b.x),f(a.y,b.y),f(a.z,b.z),f(a.w,b.w)); }    \
  
  define_functor(min);
  define_functor(max);
#undef define_functor

} // ::ospcommon
