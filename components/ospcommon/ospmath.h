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

#include <cmath>
#include "constants.h"
#include "platform.h"

#ifdef _WIN32
#include <intrin.h>
#if (__MSV_VER <= 1700)
namespace std {
  __forceinline bool isinf(const float x)
  {
    return !_finite(x);
  }
  __forceinline bool isnan(const float x)
  {
    return _isnan(x);
  }
  __forceinline bool isfinite(const float x)
  {
    return _finite(x);
  }
}  // namespace std
#endif
#else
#include <emmintrin.h>
#include <xmmintrin.h>
#endif

namespace ospcommon {

  using std::cos;
  using std::sin;
  using std::tan;

  using std::min;
  using std::max;

  using std::fmod;

  __forceinline float sign(const float x)
  {
    return x < 0 ? -1.0f : 1.0f;
  }

  __forceinline float rcp(const float x)
  {
    const __m128 a = _mm_set_ss(x);
    const __m128 r = _mm_rcp_ps(a);
    return _mm_cvtss_f32(
        _mm_mul_ps(r, _mm_sub_ps(_mm_set_ss(2.0f), _mm_mul_ps(r, a))));
  }

  __forceinline float rsqrt(const float x)
  {
    const __m128 a = _mm_set_ss(x);
    const __m128 r = _mm_rsqrt_ps(a);
    const __m128 c =
        _mm_add_ps(_mm_mul_ps(_mm_set_ps1(1.5f), r),
                   _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(a, _mm_set_ps1(-0.5f)), r),
                              _mm_mul_ps(r, r)));
    return _mm_cvtss_f32(c);
  }

  template <typename T>
  __forceinline T clamp(const T &x,
                        const T &lower = T(zero),
                        const T &upper = T(one))
  {
    return max(min(x, upper), lower);
  }

  template <typename T>
  __forceinline T deg2rad(const T &x)
  {
    return x * T(1.745329251994329576923690768489e-2);
  }

  __forceinline float madd(const float a, const float b, const float c)
  {
    return a * b + c;
  }

  template <typename T>
  inline T lerp(const float factor, const T &a, const T &b)
  {
    return (1.f - factor) * a + factor * b;
  }

  template <typename T>
  inline T divRoundUp(T a, T b)
  {
    return (a + b - 1) / b;
  }

}  // namespace ospcommon
