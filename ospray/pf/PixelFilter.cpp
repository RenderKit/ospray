// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "PixelFilter.h"
#include "common/OSPCommon_ispc.h"
#include "pf/LUTPixelFilter_ispc.h"
#include "pf/PixelFilter_ispc.h"

namespace ospray {

PixelFilter::~PixelFilter()
{
  ispc::delete_uniform(ispcEquivalent);
  ispcEquivalent = nullptr;
}

LUTPixelFilter::~LUTPixelFilter()
{
  ispc::LUTPixelFilter_destroy(ispcEquivalent);
  ispcEquivalent = nullptr;
}

PointPixelFilter::PointPixelFilter()
{
  ispcEquivalent = ispc::Point_create();
}

BoxPixelFilter::BoxPixelFilter()
{
  ispcEquivalent = ispc::Box_create();
}

GaussianLUTPixelFilter::GaussianLUTPixelFilter()
{
  ispcEquivalent = ispc::GaussianLUT_create();
}

BlackmanHarrisLUTPixelFilter::BlackmanHarrisLUTPixelFilter()
{
  ispcEquivalent = ispc::BlackmanHarrisLUT_create();
}

MitchellNetravaliLUTPixelFilter::MitchellNetravaliLUTPixelFilter()
{
  ispcEquivalent = ispc::MitchellNetravaliLUT_create();
}

} // namespace ospray
