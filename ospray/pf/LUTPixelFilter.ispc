// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "pf/LUTPixelFilter.ih"

inline vec2f LUTPixelFilter_sample(
    const PixelFilter *uniform _self, const vec2f &s)
{
  const LUTPixelFilter *uniform self = (const LUTPixelFilter *uniform)_self;

  Sample2D sample2d = Distribution2D_sample(self->distribution, s);
  return (sample2d.uv + make_vec2f(-0.5f)) * make_vec2f(self->super.width);
}

float Box_eval(const PixelFilter *uniform self, const vec2f &x)
{
  return 1.0f / (self->width * self->width);
}

inline LUTPixelFilter *uniform LUTPixelFilter_create()
{
  LUTPixelFilter *uniform self = uniform new LUTPixelFilter;
  self->super.width = 0.0f;
  self->super.sample = NULL;
  self->distribution = NULL;
  self->eval = NULL;
  return self;
}

export void LUTPixelFilter_update(void *uniform _self)
{
  LUTPixelFilter *uniform self = (LUTPixelFilter * uniform) _self;
  Distribution2D_destroy(self->distribution);
  LUTPixelFilter_buildLUT(self);
}

export void LUTPixelFilter_destroy(void *uniform _self)
{
  LUTPixelFilter *uniform self = (LUTPixelFilter * uniform) _self;
  Distribution2D_destroy(self->distribution);
  delete self;
}

export void *uniform BoxLUT_create()
{
  LUTPixelFilter *uniform self = LUTPixelFilter_create();
  LUTPixelFilter_Constructor(self, 1.0f, Box_eval);
  return self;
}

float Gaussian_eval(const PixelFilter *uniform self, const vec2f &x)
{
  const float sigma = (0.5f * self->width) / 3.0f;
  const float sigmaSqr = sigma * sigma;
  const float distanceSqr = x.x * x.x + x.y * x.y;
  return exp(-0.5f * (distanceSqr / sigmaSqr));
}

export void *uniform GaussianLUT_create()
{
  LUTPixelFilter *uniform self = LUTPixelFilter_create();
  LUTPixelFilter_Constructor(self, 3.0f, Gaussian_eval);
  return self;
}

float MitchellNetravali_eval(const PixelFilter *uniform self, const vec2f &x)
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

export void *uniform MitchellNetravaliLUT_create()
{
  LUTPixelFilter *uniform self = LUTPixelFilter_create();
  LUTPixelFilter_Constructor(self, 4.0f, MitchellNetravali_eval);
  return self;
}

float BlackmanHarris_eval(const PixelFilter *uniform self, const vec2f &x)
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

export void *uniform BlackmanHarrisLUT_create()
{
  LUTPixelFilter *uniform self = LUTPixelFilter_create();
  LUTPixelFilter_Constructor(self, 3.0f, BlackmanHarris_eval);
  return self;
}
