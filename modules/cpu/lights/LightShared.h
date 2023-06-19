// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#if defined(ISPC) || defined(OSPRAY_TARGET_SYCL)
#include "common/FeatureFlags.ih"
#endif

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

#if defined(__cplusplus) && !defined(OSPRAY_TARGET_SYCL)
typedef void *Light_SampleFunc;
typedef void *Light_EvalFunc;
#else
struct Light;
struct DifferentialGeometry;

struct Light_SampleRes
{
  vec3f weight; // radiance that arrives at the given point divided by pdf
  vec3f dir; // direction towards the light source, normalized
  float dist; // largest valid t_far value for a shadow ray, including epsilon
              // to avoid self-intersection
  float pdf; // probability density that this sample was taken
};

// compute the weighted radiance at a point caused by a sample on the light
// source
// by convention, giving (0, 0) as "random" numbers should sample the "center"
// of the light source (used by the raytracing renderers such as the SciVis
// renderer)
typedef Light_SampleRes (*Light_SampleFunc)(const Light *uniform self,
    const DifferentialGeometry &dg, // point (&normal) to generate the sample
    const vec2f &s, // random numbers to generate the sample
    const float time, // generate the sample at time (motion blur)
    const uniform FeatureFlagsHandler &ffh);

struct Light_EvalRes
{
  vec3f radiance; // radiance that arrives at the given point (not weighted by
                  // pdf)
  float pdf; // probability density that the direction would have been sampled
};

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

enum LightType
{
  LIGHT_TYPE_AMBIENT,
  LIGHT_TYPE_CYLINDER,
  LIGHT_TYPE_DIRECTIONAL,
  LIGHT_TYPE_HDRI,
  LIGHT_TYPE_POINT,
  LIGHT_TYPE_QUAD,
  LIGHT_TYPE_SPOT,
  LIGHT_TYPE_GEOMETRY,
  LIGHT_TYPE_UNKNOWN,
};

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
