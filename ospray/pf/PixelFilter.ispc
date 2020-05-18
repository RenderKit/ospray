// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "pf/PixelFilter.ih"

vec2f Point_sample(const PixelFilter *uniform self, const vec2f &s)
{
  return make_vec2f(0.0f);
}

export void *uniform Point_create()
{
  PixelFilter *uniform self = uniform new PixelFilter;
  // initializing the Point pixel filter to a width of 0 since
  // it only generates a sample at the center of the pixel
  PixelFilter_Constructor(self, 0.0f, Point_sample);
  return self;
}

vec2f Box_sample(const PixelFilter *uniform self, const vec2f &s)
{
  return (s + make_vec2f(-0.5f)) * make_vec2f(self->width);
}

export void *uniform Box_create()
{
  PixelFilter *uniform self = uniform new PixelFilter;
  // initializing the Box filter to a width of 1 pixel so that only
  // the pixel is sampled uniformly
  PixelFilter_Constructor(self, 1.0f, Box_sample);
  return self;
}
