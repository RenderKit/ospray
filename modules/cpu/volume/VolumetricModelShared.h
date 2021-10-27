// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
using namespace rkcommon::math;
namespace ispc {
#endif // __cplusplus

struct Volume;
struct TransferFunction;

struct VolumetricModel
{
  // instanced Volume
  Volume *volume;

  // Color and opacity transfer function.
  TransferFunction *transferFunction;

  // interval iterator context
  VKLIntervalIteratorContext vklIntervalContext;

  box3f boundingBox;

  // Volume parameters understood by the pathtracer
  float densityScale;
  float anisotropy; // the anisotropy of the volume's phase function
                    // (Heyney-Greenstein)
  float gradientShadingScale;
};

#ifdef __cplusplus
} // namespace ispc
#endif // __cplusplus
