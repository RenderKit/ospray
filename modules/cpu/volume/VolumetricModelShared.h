// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Volume;
struct TransferFunction;

struct VolumetricModel
{
  Volume *volume;
  TransferFunction *transferFunction;
  VKLIntervalIteratorContext vklIntervalContext;
  box3f boundingBox;

  // Volume parameters understood by the pathtracer
  float densityScale;
  float anisotropy; // the anisotropy of the volume's phase function
                    // (Heyney-Greenstein)
  float gradientShadingScale;
  unsigned int userID;

#ifdef __cplusplus
  VolumetricModel()
      : volume(nullptr),
        transferFunction(nullptr),
        vklIntervalContext(VKLIntervalIteratorContext()),
        boundingBox(0.f),
        densityScale(1.f),
        anisotropy(0.f),
        gradientShadingScale(0.f),
        userID(RTC_INVALID_GEOMETRY_ID)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
#endif
