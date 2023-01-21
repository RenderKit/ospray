// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "PixelFilter.h"
#include "ISPCDevice.h"
#include "ISPCDeviceObject.h"
#include "math/Distribution2D.h"
#ifndef OSPRAY_TARGET_SYCL
#include "pf/LUTPixelFilter_ispc.h"
#else
namespace ispc {
void LUTPixelFilter_buildLUT(void *self);
}
#endif

namespace ospray {

PixelFilter::PixelFilter(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device)
{}

LUTPixelFilter::LUTPixelFilter(const float size,
    ispc::LUTPixelFilterType lutFilterType,
    api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device)
{
  getSh()->super.width = size;
  getSh()->super.type = ispc::PIXEL_FILTER_TYPE_LUT;

  const vec2i lutSize = vec2i(std::ceil(size)) * LUTPIXELFILTER_PER_PIXEL_BINS;
  distribution = new Distribution2D(lutSize, device);
  // Remove local ref
  distribution->refDec();

  getSh()->distribution = distribution->getSh();
  getSh()->lutFilterType = lutFilterType;

  ispc::LUTPixelFilter_buildLUT(getSh());
}

PointPixelFilter::PointPixelFilter(api::ISPCDevice &device)
    : PixelFilter(device)
{
  getSh()->type = ispc::PIXEL_FILTER_TYPE_POINT;
  getSh()->width = 0.f;
}

BoxPixelFilter::BoxPixelFilter(api::ISPCDevice &device) : PixelFilter(device)
{
  getSh()->type = ispc::PIXEL_FILTER_TYPE_BOX;
  getSh()->width = 1.f;
}

GaussianLUTPixelFilter::GaussianLUTPixelFilter(api::ISPCDevice &device)
    : LUTPixelFilter(3.f, ispc::LUT_PIXEL_FILTER_TYPE_GAUSSIAN, device)

{}

BlackmanHarrisLUTPixelFilter::BlackmanHarrisLUTPixelFilter(
    api::ISPCDevice &device)
    : LUTPixelFilter(3.f, ispc::LUT_PIXEL_FILTER_TYPE_BLACKMANN_HARRIS, device)
{}

MitchellNetravaliLUTPixelFilter::MitchellNetravaliLUTPixelFilter(
    api::ISPCDevice &device)
    : LUTPixelFilter(
        4.f, ispc::LUT_PIXEL_FILTER_TYPE_MITCHELL_NETRAVALI, device)
{}

} // namespace ospray
