// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "lights/LightRes.h"
#include "lights/LightType.ih"

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
#endif // __cplusplus

#if defined(__cplusplus) && !defined(OSPRAY_TARGET_DPCPP)
typedef void *Light_SampleFunc;
typedef void *Light_EvalFunc;
#else
struct Light;
struct DifferentialGeometry;

// compute the weighted radiance at a point caused by a sample on the light
// source
// by convention, giving (0, 0) as "random" numbers should sample the "center"
// of the light source (used by the raytracing renderers such as the SciVis
// renderer)
typedef Light_SampleRes (*Light_SampleFunc)(const Light *uniform self,
    const DifferentialGeometry &dg, // point (&normal) to generate the sample
    const vec2f &s, // random numbers to generate the sample
    const float time); // generate the sample at time (motion blur)

//! compute the radiance and pdf caused by the light source (pointed to by the
//! given direction up until maxDist)
typedef Light_EvalRes (*Light_EvalFunc)(const Light *uniform self,
    const DifferentialGeometry &dg, // point to evaluate illumination for
    const vec3f &dir, // direction towards the light source, normalized
    const float minDist, // minimum distance to look for light contribution
    const float maxDist, // maximum distance to look for light contribution
    const float time); // evaluate at time (motion blur)
#endif

struct Instance;

struct Light
{
  LightType type;
  Light_SampleFunc sample;
  Light_EvalFunc eval;
  bool isVisible; // either directly in camera, or via a straight path (i.e.
                  // through ThinGlass)
  const Instance *instance;

#ifdef __cplusplus
  Light()
      : type(LIGHT_TYPE_UNKNOWN),
        sample(nullptr),
        eval(nullptr),
        isVisible(true),
        instance(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
