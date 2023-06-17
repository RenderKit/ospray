// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "pf/LUTPixelFilter.ih"
#include "pf/PixelFilter.ih"
#include "pf/PixelFilterDispatch.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec2f PixelFilter_dispatch_sample(
    const PixelFilter *uniform self, const vec2f &x)
{
  switch (self->type) {
  case PIXEL_FILTER_TYPE_POINT:
    return Point_sample(self, x);
  case PIXEL_FILTER_TYPE_BOX:
    return Box_sample(self, x);
  case PIXEL_FILTER_TYPE_LUT:
    return LUTPixelFilter_sample(self, x);
  default:
    return make_vec2f(0.f);
  }
}

OSPRAY_END_ISPC_NAMESPACE
