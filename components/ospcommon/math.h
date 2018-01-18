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

#include "platform.h"
#include "constants.h"
#include <cmath>

#ifdef _WIN32
#include <intrin.h>
#if (__MSV_VER <= 1700)
namespace std
{
  __forceinline bool isinf ( const float x ) { return !_finite(x); }
  __forceinline bool isnan ( const float x ) { return _isnan(x); }
  __forceinline bool isfinite (const float x) { return _finite(x); }
}
#endif
#else
# include <emmintrin.h>
# include <xmmintrin.h>
#endif

namespace ospcommon
{
  __forceinline bool isvalid ( const float& v ) {
    return (v > -FLT_LARGE) & (v < +FLT_LARGE);
  }

  __forceinline int cast_f2i(float f) {
    union { float f; int i; } v; v.f = f; return v.i;
  }

  __forceinline float cast_i2f(int i) {
    union { float f; int i; } v; v.i = i; return v.f;
  }

#ifdef _WIN32
  __forceinline bool finite ( const float x ) { return _finite(x) != 0; }
#endif

  __forceinline float sign ( const float x ) { return x<0?-1.0f:1.0f; }
  __forceinline float sqr  ( const float x ) { return x*x; }

  __forceinline float rcp  ( const float x )
  {
    const __m128 a = _mm_set_ss(x);
    const __m128 r = _mm_rcp_ps(a);
    return _mm_cvtss_f32(_mm_mul_ps(r,_mm_sub_ps(_mm_set_ss(2.0f), _mm_mul_ps(r, a))));
  }

  __forceinline float signmsk ( const float x ) {
    return _mm_cvtss_f32(_mm_and_ps(_mm_set_ss(x),_mm_castsi128_ps(_mm_set1_epi32(0x80000000))));
  }
  __forceinline float xorf( const float x, const float y ) {
    return _mm_cvtss_f32(_mm_xor_ps(_mm_set_ss(x),_mm_set_ss(y)));
  }
  __forceinline float andf( const float x, const unsigned y ) {
    return _mm_cvtss_f32(_mm_and_ps(_mm_set_ss(x),_mm_castsi128_ps(_mm_set1_epi32(y))));
  }
  __forceinline float rsqrt( const float x ) {
    const __m128 a = _mm_set_ss(x);
    const __m128 r = _mm_rsqrt_ps(a);
    const __m128 c = _mm_add_ps(_mm_mul_ps(_mm_set_ps1(1.5f), r),
                                _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(a, _mm_set_ps1(-0.5f)), r), _mm_mul_ps(r, r)));
    return _mm_cvtss_f32(c);
  }

#ifndef _WIN32
  __forceinline float abs  ( const float x ) { return ::fabsf(x); }
  __forceinline float acos ( const float x ) { return ::acosf (x); }
  __forceinline float asin ( const float x ) { return ::asinf (x); }
  __forceinline float atan ( const float x ) { return ::atanf (x); }
  __forceinline float atan2( const float y, const float x ) { return ::atan2f(y, x); }
  __forceinline float cos  ( const float x ) { return ::cosf  (x); }
  __forceinline float cosh ( const float x ) { return ::coshf (x); }
  __forceinline float exp  ( const float x ) { return ::expf  (x); }
  __forceinline float fmod ( const float x, const float y ) { return ::fmodf (x, y); }
  __forceinline float log  ( const float x ) { return ::logf  (x); }
  __forceinline float log10( const float x ) { return ::log10f(x); }
  __forceinline float pow  ( const float x, const float y ) { return ::powf  (x, y); }
  __forceinline float sin  ( const float x ) { return ::sinf  (x); }
  __forceinline float sinh ( const float x ) { return ::sinhf (x); }
  __forceinline float sqrt ( const float x ) { return ::sqrtf (x); }
  __forceinline float tan  ( const float x ) { return ::tanf  (x); }
  __forceinline float tanh ( const float x ) { return ::tanhf (x); }
  __forceinline float floor( const float x ) { return ::floorf (x); }
  __forceinline float ceil ( const float x ) { return ::ceilf (x); }
#endif
  __forceinline float frac ( const float x ) { return x-floor(x); }

  __forceinline double abs  ( const double x ) { return ::fabs(x); }
  __forceinline double sign ( const double x ) { return x<0?-1.0:1.0; }
  __forceinline double acos ( const double x ) { return ::acos (x); }
  __forceinline double asin ( const double x ) { return ::asin (x); }
  __forceinline double atan ( const double x ) { return ::atan (x); }
  __forceinline double atan2( const double y, const double x ) { return ::atan2(y, x); }
  __forceinline double cos  ( const double x ) { return ::cos  (x); }
  __forceinline double cosh ( const double x ) { return ::cosh (x); }
  __forceinline double exp  ( const double x ) { return ::exp  (x); }
  __forceinline double fmod ( const double x, const double y ) { return ::fmod (x, y); }
  __forceinline double log  ( const double x ) { return ::log  (x); }
  __forceinline double log10( const double x ) { return ::log10(x); }
  __forceinline double pow  ( const double x, const double y ) { return ::pow  (x, y); }
  __forceinline double rcp  ( const double x ) { return 1.0/x; }
  __forceinline double rsqrt( const double x ) { return 1.0/::sqrt(x); }
  __forceinline double sin  ( const double x ) { return ::sin  (x); }
  __forceinline double sinh ( const double x ) { return ::sinh (x); }
  __forceinline double sqr  ( const double x ) { return x*x; }
  __forceinline double sqrt ( const double x ) { return ::sqrt (x); }
  __forceinline double tan  ( const double x ) { return ::tan  (x); }
  __forceinline double tanh ( const double x ) { return ::tanh (x); }
  __forceinline double floor( const double x ) { return ::floor (x); }
  __forceinline double ceil ( const double x ) { return ::ceil (x); }

  __forceinline     int min(int     a, int     b) { return a<b ? a:b; }
  __forceinline int64_t min(int64_t a, int64_t b) { return a<b ? a:b; }
  __forceinline  size_t min(size_t  a, size_t  b) { return a<b ? a:b; }
  __forceinline   float min(float   a, float   b) { return a<b ? a:b; }
  __forceinline  double min(double  a, double  b) { return a<b ? a:b; }

  template<typename T> __forceinline T min(const T& a, const T& b, const T& c) { return min(min(a,b),c); }
  template<typename T> __forceinline T min(const T& a, const T& b, const T& c, const T& d) { return min(min(a,b),min(c,d)); }
  template<typename T> __forceinline T min(const T& a, const T& b, const T& c, const T& d, const T& e) { return min(min(min(a,b),min(c,d)),e); }

  __forceinline     int max(int     a, int     b) { return a<b ? b:a; }
  __forceinline int64_t max(int64_t a, int64_t b) { return a<b ? b:a; }
  __forceinline  size_t max(size_t  a, size_t  b) { return a<b ? b:a; }
  __forceinline   float max(float   a, float   b) { return a<b ? b:a; }
  __forceinline  double max(double  a, double  b) { return a<b ? b:a; }

  template<typename T> __forceinline T max(const T& a, const T& b, const T& c) { return max(max(a,b),c); }
  template<typename T> __forceinline T max(const T& a, const T& b, const T& c, const T& d) { return max(max(a,b),max(c,d)); }
  template<typename T> __forceinline T max(const T& a, const T& b, const T& c, const T& d, const T& e) { return max(max(max(a,b),max(c,d)),e); }

#ifdef __APPLE__
  __forceinline ssize_t min(ssize_t a, ssize_t b) { return a<b ? a:b; }
  __forceinline ssize_t max(ssize_t a, ssize_t b) { return a<b ? b:a; }
#endif

  template<typename T> __forceinline T clamp(const T& x, const T& lower = T(zero), const T& upper = T(one)) { return max(min(x,upper),lower); }
  template<typename T> __forceinline T clampz(const T& x, const T& upper) { return max(T(zero), min(x,upper)); }

  template<typename T> __forceinline T  deg2rad ( const T& x )  { return x * T(1.745329251994329576923690768489e-2); }
  template<typename T> __forceinline T  rad2deg ( const T& x )  { return x * T(5.7295779513082320876798154814105e1); }
  template<typename T> __forceinline T  sin2cos ( const T& x )  { return sqrt(max(T(zero),T(one)-x*x)); }
  template<typename T> __forceinline T  cos2sin ( const T& x )  { return sin2cos(x); }

  __forceinline float madd  ( const float a, const float b, const float c) { return a*b+c; }
  __forceinline float msub  ( const float a, const float b, const float c) { return a*b-c; }
  __forceinline float nmadd ( const float a, const float b, const float c) { return -a*b+c;}
  __forceinline float nmsub ( const float a, const float b, const float c) { return -a*b-c; }

  /*! random functions */
  template<typename T> T random() { return T(0); }
#ifdef _WIN32
  template<> __forceinline int      random() { return int(rand()) ^ (int(rand()) << 8) ^ (int(rand()) << 16); }
  template<> __forceinline uint32_t random() { return uint32_t(rand()) ^ (uint32_t(rand()) << 8) ^ (uint32_t(rand()) << 16); }
#else
  template<> __forceinline int      random() { return int(rand()); }
  template<> __forceinline uint32_t random() { return uint32_t(rand()) ^ (uint32_t(rand()) << 16); }
#endif
  template<> __forceinline float  random() { return rand()/float(RAND_MAX); }
  template<> __forceinline double random() { return rand()/double(RAND_MAX); }

#ifdef _WIN32
  __forceinline double drand48() {
    return double(rand())/double(RAND_MAX);
  }

  __forceinline void srand48(long seed) {
    return srand(seed);
  }
#endif

  template <typename T>
  inline T select(const bool s, const T &t, const T &f)
  {
    return s ? t : f;
  }

  template <typename T>
  inline T lerp(const float factor, const T &a, const T &b)
  {
     return (1.f - factor)*a + factor*b;
  }


  template<typename T>
    __forceinline T lerp2(const float x0, const float x1, const float x2, const float x3,const T &u, const T &v) {
    return (1.0f-u)*(1.0f-v)*x0 + u*(1.0f-v)*x1 + (1.0f-u)*v*x2 + u*v*x3;
  }

  // -------------------------------------------------------
  // scalar functors we eventually define for vec's, too
  // -------------------------------------------------------
  template<typename T>
  inline T divRoundUp(T a, T b) { return (a+b-1)/b; }

  /*! exchange */
  template<typename T> __forceinline void xchg ( T& a, T& b ) { const T tmp = a; a = b; b = tmp; }

  /*! bit reverse operation */
  template<class T>
    __forceinline T bitReverse(const T& vin)
  {
    T v = vin;
    v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
    v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
    v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
    v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
    v = ( v >> 16             ) | ( v               << 16);
    return v;
  }

  /*! bit interleave operation */
  template<class T>
    __forceinline T bitInterleave(const T& xin, const T& yin, const T& zin)
  {
	T x = xin, y = yin, z = zin;
    x = (x | (x << 16)) & 0x030000FF;
    x = (x | (x <<  8)) & 0x0300F00F;
    x = (x | (x <<  4)) & 0x030C30C3;
    x = (x | (x <<  2)) & 0x09249249;

    y = (y | (y << 16)) & 0x030000FF;
    y = (y | (y <<  8)) & 0x0300F00F;
    y = (y | (y <<  4)) & 0x030C30C3;
    y = (y | (y <<  2)) & 0x09249249;

    z = (z | (z << 16)) & 0x030000FF;
    z = (z | (z <<  8)) & 0x0300F00F;
    z = (z | (z <<  4)) & 0x030C30C3;
    z = (z | (z <<  2)) & 0x09249249;

    return x | (y << 1) | (z << 2);
  }

  /*! bit interleave operation for 64bit data types*/
  template<class T>
    __forceinline T bitInterleave64(const T& xin, const T& yin, const T& zin){
    T x = xin & 0x1fffff;
    T y = yin & 0x1fffff;
    T z = zin & 0x1fffff;

    x = (x | x << 32) & 0x1f00000000ffff;
    x = (x | x << 16) & 0x1f0000ff0000ff;
    x = (x | x << 8) & 0x100f00f00f00f00f;
    x = (x | x << 4) & 0x10c30c30c30c30c3;
    x = (x | x << 2) & 0x1249249249249249;

    y = (y | y << 32) & 0x1f00000000ffff;
    y = (y | y << 16) & 0x1f0000ff0000ff;
    y = (y | y << 8) & 0x100f00f00f00f00f;
    y = (y | y << 4) & 0x10c30c30c30c30c3;
    y = (y | y << 2) & 0x1249249249249249;

    z = (z | z << 32) & 0x1f00000000ffff;
    z = (z | z << 16) & 0x1f0000ff0000ff;
    z = (z | z << 8) & 0x100f00f00f00f00f;
    z = (z | z << 4) & 0x10c30c30c30c30c3;
    z = (z | z << 2) & 0x1249249249249249;

    return x | (y << 1) | (z << 2);
  }
}
