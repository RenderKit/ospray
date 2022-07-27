// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/rkmath.h"
// ispc shared
#include "GeometryLightShared.h"
#include "PathTracerDataShared.h"

using namespace ospray;

namespace ispc {

void PathtracerData::create(
    Light **lights, uint32 numLights, uint32 numGeoLights, float *lightsCDF)
{
  this->numLights = numLights;
  this->numGeoLights = numGeoLights;
  if (numLights) {
    // Allocate shared buffers and copy light pointers there
    this->lights = BufferSharedCreate<Light *>(numLights, lights);
    this->lightsCDF = BufferSharedCreate<float>(numLights, lightsCDF);
  }
}

void PathtracerData::destroy()
{
  // Delete GeometryLights first
  for (unsigned int i = 0; i < numGeoLights; i++) {
    ((GeometryLight *)lights[i])->destroy();
    BufferSharedDelete(lights[i]);
  }

  // Delete remaining shared lights structures
  for (unsigned int i = numGeoLights; i < numLights; i++)
    BufferSharedDelete(lights[i]);

  // Release array memory
  BufferSharedDelete(lights);
  BufferSharedDelete(lightsCDF);
  lights = nullptr;
  lightsCDF = nullptr;
}
} // namespace ispc