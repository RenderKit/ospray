// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/Distribution2DShared.h"
#include "pf/PixelFilterShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

enum LUTPixelFilterType
{
  LUT_PIXEL_FILTER_TYPE_GAUSSIAN,
  LUT_PIXEL_FILTER_TYPE_BLACKMANN_HARRIS,
  LUT_PIXEL_FILTER_TYPE_MITCHELL_NETRAVALI,
};

#define LUTPIXELFILTER_PER_PIXEL_BINS 5

struct LUTPixelFilter
{
  PixelFilter super;

  // The 2D distribution used to importance sample
  Distribution2D *distribution;

  LUTPixelFilterType lutFilterType;

#ifdef __cplusplus
  LUTPixelFilter()
      : distribution(nullptr), lutFilterType{LUT_PIXEL_FILTER_TYPE_GAUSSIAN}
  {
    super.type = PIXEL_FILTER_TYPE_LUT;
  }
};
}
#else
};
#endif
