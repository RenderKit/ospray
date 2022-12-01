// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/Distribution1D.ih"
#include "pf/LUTPixelFilter.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

vec2f LUTPixelFilter_sample(const PixelFilter *uniform _self, const vec2f &s)
{
  const LUTPixelFilter *uniform self = (const LUTPixelFilter *uniform)_self;

  Sample2D sample2d = Distribution2D_sample(self->distribution, s);
  return (sample2d.uv + make_vec2f(-0.5f)) * make_vec2f(self->super.width);
}

inline float Gaussian_eval(const PixelFilter *uniform self, const vec2f &x)
{
  const float sigma = (0.5f * self->width) / 3.0f;
  const float sigmaSqr = sigma * sigma;
  const float distanceSqr = x.x * x.x + x.y * x.y;
  return exp(-0.5f * (distanceSqr / sigmaSqr));
}

inline float MitchellNetravali_eval(
    const PixelFilter *uniform self, const vec2f &x)
{
  const float oneOverSix = 1.0f / 6.0f;
  const float B = 1.0f / 3.0f;
  const float C = 1.0f / 3.0f;

  const vec2f tx = (x / make_vec2f(self->width)) * make_vec2f(4.0f);
  const float fx = sqrt(tx.x * tx.x + tx.y * tx.y);
  const float fx2 = fx * fx;
  const float fx3 = fx2 * fx;

  float value = 0.0f;
  if (fx < 1.0f) {
    value = oneOverSix
        * ((12.0f - 9.0f * B - 6.0f * C) * fx3
            + (-18.0f + 12.0f * B + 6.0f * C) * fx2 + (6.0f - 2.0f * B));
  } else if (fx < 2.0f) {
    value = oneOverSix
        * ((-B - 6.0f * C) * fx3 + (6.0f * B + 30.0f * C) * fx2
            + (-12.0f * B - 48.0f * C) * fx + (8.0f * B + 24.0f * C));
  }
  return value;
}

inline float BlackmanHarris_eval(
    const PixelFilter *uniform self, const vec2f &x)
{
  const float a0 = 0.35875f;
  const float a1 = 0.48829f;
  const float a2 = 0.14128f;
  const float a3 = 0.01168f;

  const float fx = sqrt(x.x * x.x + x.y * x.y) + self->width / 2.0f;
  const float oneOverWMinusOne = 1.0f / (self->width);
  const float value = a0 - a1 * cos((2.0f * M_PI * fx) * oneOverWMinusOne)
      + a2 * cos((4.0f * M_PI * fx) * oneOverWMinusOne)
      - a3 * cos((6.0f * M_PI * fx) * oneOverWMinusOne);

  return value;
}

#ifndef OSPRAY_TARGET_SYCL
task
#endif
    unmasked void
    LUTPixelFilter_calcRowImportance(const LUTPixelFilter *uniform const self,
        int uniform const lut_width,
        float *uniform const importance,
        float *uniform const row_importance
#ifdef OSPRAY_TARGET_SYCL
        ,
        const int taskIndex
#endif
    )
{
  const uniform float filter_width = self->super.width;

  const uniform int y = taskIndex;
  const uniform float fy = (y + 0.5f) * (1.0f / (float)lut_width);

#ifdef OSPRAY_TARGET_SYCL
  for (int x = 0; x < lut_width; ++x) {
#else
  foreach (x = 0 ... lut_width) {
#endif
    const varying float fx = (x + 0.5f) * (1.0f / (float)lut_width);
    const vec2f coord = make_vec2f(fx, fy) - make_vec2f(0.5f);

    float filterEval = 0.f;
    switch (self->lutFilterType) {
    case LUT_PIXEL_FILTER_TYPE_GAUSSIAN:
      filterEval =
          Gaussian_eval(&self->super, coord * make_vec2f(filter_width));
      break;
    case LUT_PIXEL_FILTER_TYPE_MITCHELL_NETRAVALI:
      filterEval = MitchellNetravali_eval(
          &self->super, coord * make_vec2f(filter_width));
      break;
    case LUT_PIXEL_FILTER_TYPE_BLACKMANN_HARRIS:
      filterEval =
          BlackmanHarris_eval(&self->super, coord * make_vec2f(filter_width));
      break;
    default:
      break;
    }

    // clamp the evaluation of the filter
    // to avoid negative values
    importance[y * lut_width + x] = max(0.0f, filterEval);
  }
  row_importance[y] =
      Distribution1D_create(lut_width, importance + y * lut_width);
}

export void LUTPixelFilter_buildLUT(void *uniform _self)
{
  LUTPixelFilter *uniform self = (LUTPixelFilter * uniform) _self;
  const uniform int lut_width = self->distribution->size.x;

  float *uniform cdf_x = self->distribution->cdf_x;
  float *uniform row_importance = self->distribution->cdf_y;
#ifdef OSPRAY_TARGET_SYCL
  // TODO: This could become a kernel launch via SYCL
  // This serial loop is just for testing an initial C++ migration of the ISPC
  // code
  for (int i = 0; i < lut_width; ++i) {
    LUTPixelFilter_calcRowImportance(self, lut_width, cdf_x, row_importance, i);
  }
#else
  launch[lut_width] LUTPixelFilter_calcRowImportance(
      self, lut_width, cdf_x, row_importance);
  sync;
#endif

  // Initialize the distribution with the computed cdf/f values
  Distribution2D_init(self->distribution);
}

OSPRAY_END_ISPC_NAMESPACE
