// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/World.ih"
#include "render/pathtracer/PathStructs.ih"
#include "render/pathtracer/PathTracerUtil.ih"
#include "render/pathtracer/VirtualLight.ih"
#ifdef OSPRAY_ENABLE_VOLUMES
#include "render/pathtracer/volumes/VolumeSampler.ih"
#endif
// c++ shared
#include "lights/LightDispatch.ih"
#include "lights/LightShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

inline box1f getMinMaxDistForVirtualLights(
    const PathVertex &lastVertex, const PathVertex &pathVertex, const Ray &ray)
{
  box1f interval;

  // minimal distance is not always zero (when previously sampled a Dirac
  // transmission)
  interval.lower = distance(lastVertex.dg.P, ray.org);

  // virtual lights are occluded by hit geometry
  // because lastVertex.dg.P can be different from ray.org (when previously
  // sampled a Dirac transmission) we cannot just use ray.t as maximum distance
  interval.upper = distance(lastVertex.dg.P, ray.org + ray.t * ray.dir);

  if (pathVertex.type == ENVIRONMENT)
    interval.upper = inf;

  return interval;
}

// add light from *virtual* lights by intersecting them
SYCL_EXTERNAL vec3f evaluateVirtualLights(const PathContext &pathContext,
    PathState &pathState,
    const PathVertex &lastVertex,
    const PathVertex &pathVertex,
    Ray &ray,
    const uniform FeatureFlagsHandler &ffh)
{
  vec3f L = make_vec3f(0.f);

  if (pathContext.numLights <= pathContext.numGeoLights)
    return L; // return if no virtual lights at all

  box1f intervalLightDist =
      getMinMaxDistForVirtualLights(lastVertex, pathVertex, ray);

  // at the moment we only support uniform light selection
  // therefore we can evaluate the selection PDF directly
  const float selectionPDF =
      lastVertex.numLightSamples / (float)pathContext.numLights;
  for (uniform uint32 i = pathContext.numGeoLights; i < pathContext.numLights;
       i++) {
    const Light *uniform light = pathContext.lights[i];
    if (!pathState.straightPath || light->isVisible) {
      // to correctly handle MIS through transparencies the light pdf needs to
      // be calculated wrt. lastVertex.dg
      // however, we only have a valid intersection with the light in
      // [minLightDist, maxLightDist], otherwise light could be added twice
      Light_EvalRes le = Light_dispatch_eval(light,
          lastVertex.dg,
          ray.dir,
          intervalLightDist.lower,
          intervalLightDist.upper,
          pathState.time,
          ffh);
      if (reduce_max(le.radiance) > 0.0f) {
        float misWeight = 1.0f;
        if (selectionPDF) {
          float T = 1.f;
#ifdef OSPRAY_ENABLE_VOLUMES
          Ray shadowRay;
          setRay(shadowRay,
              lastVertex.dg.P,
              ray.dir,
              intervalLightDist.lower,
              intervalLightDist.upper,
              ray.time);

          // Trace ray in clipping geometries scene, fill array with ray
          // intervals
          RayIntervals rayIntervals;
          traceClippingRay(pathContext.world, shadowRay, rayIntervals, ffh);

          const uniform FeatureFlags ff = getFeatureFlags(ffh);
          if (ff.other & FFO_VOLUME_IN_SCENE) {
            T = volumeTransmittance(pathContext.world,
                shadowRay,
                rayIntervals,
                &pathState.randomSampler,
                ffh);
          }
#endif
          misWeight = misHeuristic(
              pathContext, lastVertex.pdf_w * T, selectionPDF * le.pdf);
        }
        L = L + pathState.throughput * le.radiance * misWeight;
      }
    }
  }
  return L;
}

OSPRAY_END_ISPC_NAMESPACE
