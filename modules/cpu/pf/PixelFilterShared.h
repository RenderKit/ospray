// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

#if defined(__cplusplus) && !defined(OSPRAY_TARGET_SYCL)
typedef void *PixelFilter_SampleFunc;
#else
struct PixelFilter;

/* Samples a relative pixel position proportional to the kernel of a
   pixel filter.

   The sampled position is relative to the center of a pixel. It converts a
   uniform 2D random variable s with values of the range [0..1) into a
   non-uniform 2D position in the range of [-w/2 .. w/2], where w =
   self->width. The center of the pixel filter kernel is [0,0], which
   represents a position at the center of the pixel.

   Returns a 2D position in the domain [-w/2 .. w/2] distributed proportionally
   to the filter kernel.
*/
typedef vec2f (*PixelFilter_SampleFunc)(
    const PixelFilter *uniform self, const vec2f &s);
#endif

enum PixelFilterType
{
  PIXEL_FILTER_TYPE_POINT,
  PIXEL_FILTER_TYPE_BOX,
  PIXEL_FILTER_TYPE_LUT,
  PIXEL_FILTER_TYPE_UNKNOWN,
};

struct PixelFilter
{
  PixelFilterType type;
  float width;
  PixelFilter_SampleFunc sample;

#ifdef __cplusplus
  PixelFilter() : type(PIXEL_FILTER_TYPE_UNKNOWN), width(0.f), sample(nullptr)
  {}
};
}
#else
};
#endif
