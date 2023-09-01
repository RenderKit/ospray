// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDevice.h"
#include "ISPCDeviceObject.h"
#include "common/Managed.h"
#include "common/StructShared.h"
#include "math/Distribution2D.h"
// ispc shared
#include "pf/LUTPixelFilterShared.h"
#include "pf/PixelFilterShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE PixelFilter
    : public AddStructShared<ISPCDeviceObject, ispc::PixelFilter>
{
  PixelFilter(api::ISPCDevice &device);
};

struct OSPRAY_SDK_INTERFACE LUTPixelFilter
    : public AddStructShared<PixelFilter, ispc::LUTPixelFilter>
{
  LUTPixelFilter(const float size,
      ispc::LUTPixelFilterType lutFilterType,
      api::ISPCDevice &device);

  Ref<Distribution2D> distribution;
};

struct OSPRAY_SDK_INTERFACE PointPixelFilter : public PixelFilter
{
  PointPixelFilter(api::ISPCDevice &device);
};

struct OSPRAY_SDK_INTERFACE BoxPixelFilter : public PixelFilter
{
  BoxPixelFilter(api::ISPCDevice &device);
};

struct OSPRAY_SDK_INTERFACE GaussianLUTPixelFilter : public LUTPixelFilter
{
  GaussianLUTPixelFilter(api::ISPCDevice &device);
};

struct OSPRAY_SDK_INTERFACE BlackmanHarrisLUTPixelFilter : public LUTPixelFilter
{
  BlackmanHarrisLUTPixelFilter(api::ISPCDevice &device);
};

struct OSPRAY_SDK_INTERFACE MitchellNetravaliLUTPixelFilter
    : public LUTPixelFilter
{
  MitchellNetravaliLUTPixelFilter(api::ISPCDevice &device);
};

} // namespace ospray
