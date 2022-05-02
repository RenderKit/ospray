// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/World.ih"
#include "render/pathtracer/PathStructs.ih"
#include "render/pathtracer/PathTracerUtil.ih"
#include "render/pathtracer/VirtualLight.ih"
#include "render/pathtracer/volumes/VolumeSampler.ih"
// c++ shared
#include "lights/LightShared.h"

inline box1f getMinMaxDistForVirtualLights(const PathContext &pathContext,
    const PathState &pathState,
    const PathVertex &lastVertex,
    const PathVertex &pathVertex,
    const Ray &ray)
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

vec3f evaluateVirtualLights(const PathContext &pathContext,
    const PathState &pathState,
    const PathVertex &lastVertex,
    const PathVertex &pathVertex,
    Ray &ray)
{
  // add light from *virtual* lights by intersecting them
  vec3f L = make_vec3f(0.f);

  const World *uniform w = pathContext.world;
  if (pathContext.numLights <= w->pathtracerData.numGeoLights)
    return L; // return if no virtual lights at all

  box1f intervalLightDist = getMinMaxDistForVirtualLights(
      pathContext, pathState, lastVertex, pathVertex, ray);

  const uniform float selectionPDF = rcp(
      (uniform float)pathContext.numLights - w->pathtracerData.numGeoLights);
  for (uniform int i = w->pathtracerData.numGeoLights;
       i < pathContext.numLights;
       i++) {
    const Light *uniform light = w->pathtracerData.lights[i];
    if (!pathState.straightPath || light->isVisible) {
      // to correctly handle MIS through transparencies the light pdf needs to
      // be calculated wrt. lastVertex.dg however, we only have a valid
      // intersection with the light in [minLightDist, maxLightDist], otherwise
      // light could be added twice
      Light_EvalRes le = light->eval(light,
          lastVertex.dg,
          ray.dir,
          intervalLightDist.lower,
          intervalLightDist.upper,
          pathState.time);
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
        traceClippingRay(pathContext.world, shadowRay, rayIntervals);

        const float T = volumeTransmittance(pathState,
            pathContext.world,
            shadowRay,
            rayIntervals,
            pathContext.ldSampler,
            pathContext.randomSampler);
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
