// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Distribution2D.h"

namespace ospray {
Distribution2D::Distribution2D(const vec2i &size, api::ISPCDevice &device)
    : AddStructShared(device.getDRTDevice(), device)
{
  getSh()->size = size;
  getSh()->rcpSize = vec2f(1.f / size.x, 1.f / size.y);

  cdf_x = devicert::make_buffer_shared_unique<float>(
      device.getDRTDevice(), size.x * size.y);
  cdf_y =
      devicert::make_buffer_shared_unique<float>(device.getDRTDevice(), size.y);

  getSh()->cdf_x = cdf_x->data();
  getSh()->cdf_y = cdf_y->data();
}
} // namespace ospray
