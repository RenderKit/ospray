// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/Data.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE IntensityDistribution
{
  void readParams(ManagedObject &);

  operator bool() const
  {
    return lid;
  }
  float *data() const
  {
    return lid ? lid->data() : nullptr;
  }

  vec2i size{0};
  vec3f c0{1.f, 0.f, 0.f};

 private:
  Ref<const DataT<float, 2>> lid; // luminous intensity distribution
};

} // namespace ospray
