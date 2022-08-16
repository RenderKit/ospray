// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Distribution1D.ih"
#include "Distribution2D.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL Sample2D Distribution2D_sample(
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

SYCL_EXTERNAL float Distribution2D_pdf(
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

SYCL_EXTERNAL void Distribution2D_init(Distribution2D *uniform self)
{
  // initialize the y distribution
  Distribution1D_create(self->size.y, self->cdf_y);
}

OSPRAY_END_ISPC_NAMESPACE
