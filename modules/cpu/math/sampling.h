// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// utility library containing sampling functions

// convention is to return the sample (vec3f) generated from given vec2f
// 's'ample as last parameter sampling functions often come in pairs: sample and
// pdf (needed later for MIS) good reference is "Total Compendium" by Philip
// Dutre http://people.cs.kuleuven.be/~philip.dutre/GI/

#include "rkcommon/math/rkmath.h"

namespace ospray {

inline float uniformSampleConePDF(const float cosAngle)
{
  return rcp(float(two_pi) * (1.0f - cosAngle));
}

inline float uniformSampleRingPDF(const float radius, const float innerRadius)
{
  return rcp(float(pi) * ((radius * radius) - (innerRadius * innerRadius)));
}

} // namespace ospray
