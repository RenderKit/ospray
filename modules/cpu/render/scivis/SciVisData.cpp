// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/rkmath.h"
#include "rkcommon/math/vec.h"
// ispc shared
#include "SciVisDataShared.h"
#include "lights/LightShared.h"

using namespace ospray;

namespace ispc {

void SciVisData::create(Light **lightsPtr,
    uint32 numLights,
    uint32 numLightsVisibleOnly,
    const vec3f &aoColor)
{
  this->numLights = numLights;
  this->numLightsVisibleOnly = numLightsVisibleOnly;
  aoColorPi = aoColor * float(pi);
  if (numLights) {
    // Allocate shared buffer and copy light pointers there
    lights = BufferSharedCreate<Light *>(numLights, lightsPtr);
  }
}

void SciVisData::destroy()
{
  // Delete all lights structures
  for (unsigned int i = 0; i < numLights; i++)
    BufferSharedDelete(lights[i]);

  // Release shared arrays
  BufferSharedDelete(lights);
  lights = nullptr;
}
} // namespace ispc
