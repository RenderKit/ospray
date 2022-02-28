// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/rkmath.h"
// ispc exports
#include "render/pathtracer/GeometryLight_ispc.h"
// ispc shared
#include "GeometryLightShared.h"

namespace ispc {

using namespace ospray;

void GeometryLight::create(const Instance *instance,
    const GeometricModel *model,
    int32 numPrimitives,
    int32 *primIDs,
    float *distribution,
    float pdf)
{
  super.instance = instance;
  super.sample = ispc::GeometryLight_sample_addr();
  super.eval = nullptr; // geometry lights are hit and explicitly handled

  this->model = model;
  this->numPrimitives = numPrimitives;
  this->pdf = pdf;

  this->primIDs = BufferSharedCreate<int32>(numPrimitives, primIDs);
  this->distribution = BufferSharedCreate<float>(numPrimitives, distribution);
}

void GeometryLight::destroy()
{
  BufferSharedDelete(primIDs);
  BufferSharedDelete(distribution);
  primIDs = nullptr;
  distribution = nullptr;
}
} // namespace ispc