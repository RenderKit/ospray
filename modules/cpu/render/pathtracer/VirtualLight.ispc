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
#include "PathTracerDataShared.h"
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

SYCL_EXTERNAL vec3f evaluateVirtualLights(const PathContext &pathContext,
    const PathState &pathState,
    const PathVertex &lastVertex,
    const PathVertex &pathVertex,
    Ray &ray,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlags ff = getFeatureFlags(ffh);

  // add light from *virtual* lights by intersecting them
  vec3f L = make_vec3f(0.f);

  const World *uniform w = pathContext.world;
  const uniform PathTracerData &pathtracerData =
      *((const uniform PathTracerData *)w->pathtracerData);
  if (pathContext.numLights <= pathtracerData.numGeoLights)
    return L; // return if no virtual lights at all

  box1f intervalLightDist =
      getMinMaxDistForVirtualLights(lastVertex, pathVertex, ray);

  const uniform float selectionPDF =
      rcp((uniform float)pathContext.numLights - pathtracerData.numGeoLights);
  for (uniform uint32 i = pathtracerData.numGeoLights;
       i < pathContext.numLights;
       i++) {
    const Light *uniform light = pathtracerData.lights[i];
    if (!pathState.straightPath || light->isVisible) {
      // to correctly handle MIS through transparencies the light pdf needs to
      // be calculated wrt. lastVertex.dg however, we only have a valid
      // intersection with the light in [minLightDist, maxLightDist], otherwise
      // light could be added twice
      Light_EvalRes le = Light_dispatch_eval(light,
          lastVertex.dg,
          ray.dir,
          intervalLightDist.lower,
          intervalLightDist.upper,
          pathState.time,
          ffh);
      if (reduce_max(le.radiance) > 0.0f) {
        Ray shadowRay;
        setRay(shadowRay,
            lastVertex.dg.P,
            ray.dir,
            intervalLightDist.lower,
            intervalLightDist.upper,
            ray.time);

        // Trace ray in clipping geometries scene, fill array with ray intervals
        RayIntervals rayIntervals;
        traceClippingRay(pathContext.world, shadowRay, rayIntervals, ffh);

        float T = 1.f;
#ifdef OSPRAY_ENABLE_VOLUMES
        if (ff.other & FFO_VOLUME_IN_SCENE) {
          T = volumeTransmittance(pathContext.world,
              shadowRay,
              rayIntervals,
              pathContext.randomSampler,
              ffh);
        }
#endif
        L = L
            + pathState.throughput * le.radiance
                * misHeuristic(pathState,
                    lastVertex.pdf_w * T,
                    selectionPDF * pathContext.numLightSamples * le.pdf);
      }
    }
  }
  return L;
}

OSPRAY_END_ISPC_NAMESPACE
