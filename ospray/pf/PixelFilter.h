// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/Managed.h"
#include "common/Util.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE PixelFilter
{
  PixelFilter() = default;

  virtual ~PixelFilter();

  void *getIE()
  {
    return ispcEquivalent;
  }

 protected:
  void *ispcEquivalent{nullptr};
};

struct OSPRAY_SDK_INTERFACE LUTPixelFilter : public PixelFilter
{
  ~LUTPixelFilter() override;
};

struct OSPRAY_SDK_INTERFACE PointPixelFilter : public PixelFilter
{
  PointPixelFilter();
  ~PointPixelFilter() override = default;
};

struct OSPRAY_SDK_INTERFACE BoxPixelFilter : public PixelFilter
{
  BoxPixelFilter();
  ~BoxPixelFilter() override = default;
};

struct OSPRAY_SDK_INTERFACE GaussianLUTPixelFilter : public LUTPixelFilter
{
  GaussianLUTPixelFilter();
  ~GaussianLUTPixelFilter() override = default;
};

struct OSPRAY_SDK_INTERFACE BlackmanHarrisLUTPixelFilter : public LUTPixelFilter
{
  BlackmanHarrisLUTPixelFilter();
  ~BlackmanHarrisLUTPixelFilter() override = default;
};

struct OSPRAY_SDK_INTERFACE MitchellNetravaliLUTPixelFilter
    : public LUTPixelFilter
{
  MitchellNetravaliLUTPixelFilter();
  ~MitchellNetravaliLUTPixelFilter() override = default;
};

} // namespace ospray
