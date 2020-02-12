// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Distribution2D.ih"

Sample2D Distribution2D_sample(
    const uniform Distribution2D *uniform self, const vec2f &s)
{
  // use u.y to sample a row
  const Sample1D sy = Distribution1D_sample(self->size.y, self->cdf_y, 0, s.y);

  // use u.x to sample inside the row
  const int x0 = sy.idx * self->size.x;
  const Sample1D sx = Distribution1D_sample(self->size.x, self->cdf_x, x0, s.x);

  Sample2D ret;
  ret.uv = make_vec2f((sx.idx + sx.frac) * self->rcpSize.x,
      (sy.idx + sy.frac) * self->rcpSize.y);
  ret.pdf = sx.pdf * sy.pdf;

  return ret;
}

float Distribution2D_pdf(
    const uniform Distribution2D *uniform self, const vec2f &uv)
{
  const int idy = uv.y * self->size.y;
  const float befy = idy == 0 ? 0.0f : self->cdf_y[idy - 1];
  const float pdf_y = (self->cdf_y[idy] - befy) * self->size.y;

  const int idx = uv.x * self->size.x;
  const int x0 = idy * self->size.x;
  const float befx = idx == 0 ? 0.0f : self->cdf_x[x0 + idx - 1];
  const float pdf_x = (self->cdf_x[x0 + idx] - befx) * self->size.x;

  return pdf_x * pdf_y;
}

void Distribution2D_destroy(Distribution2D *uniform self)
{
  if (self != NULL) {
    delete[] self->cdf_x;
    delete[] self->cdf_y;
    delete self;
  }
}

Distribution2D *uniform Distribution2D_create(
    const uniform vec2i size, float *uniform cdf_x, float *uniform f_y)
{
  Distribution2D *uniform self = uniform new Distribution2D;

  self->size = size;
  self->rcpSize = make_vec2f(1.0f / size.x, 1.0f / size.y);
  self->cdf_x = cdf_x;

  // initialize the y distribution
  Distribution1D_create(size.y, f_y);
  self->cdf_y = f_y;

  return self;
}

Distribution2D *uniform Distribution2D_create(
    const uniform vec2i size, float *uniform f)
{
  // create cdf for sampling rows
  float *uniform row_importance = uniform new float[size.y];

  // compute y distribution and initialize row distributions
  for (uniform int y = 0; y < size.y; y++)
    row_importance[y] = Distribution1D_create(size.x, f + y * size.x);

  return Distribution2D_create(size, f, row_importance);
}
