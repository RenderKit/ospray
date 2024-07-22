// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/pathtracer/PathStructs.ih"
#include "render/pathtracer/PathTracerDefines.ih"
#include "render/pathtracer/ShadowCatcher.ih"
#include "render/pathtracer/TransparentShadow.ih"

#include "render/Renderer.ih"

#include "common/Ray.ih"
#include "common/World.ih"
#include "math/random.ih"
// c++ shared
#include "PathTracerDataShared.h"
#include "lights/LightDispatch.ih"
#include "lights/LightShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL bool shadowCatcher(const PathContext &pathContext,
    PathState &pathState,
    PathVertex &pathVertex,
    const Ray &ray,
    const RayCone &rayCone,
    const uniform FeatureFlagsHandler &ffh)

{
  if (!(pathContext.context->backgroundRefraction
              ? pathState.specularTransmissionPath
              : pathState.straightPath))
    return false;

  // TODO use MIS as well
  // consider real (flagged) geometries with material and move into light loop
  // (will also handle MIS)
  if (pathState.shadowCatcherDist <= ray.t
      && pathState.shadowCatcherDist > ray.t0) {
    // "postIntersect" of shadowCatcher plane
    pathVertex.dg.Ns = pathVertex.dg.Ng =
        make_vec3f(pathContext.context->shadowCatcherPlane);
    if (dot(ray.dir, pathVertex.dg.Ng) >= 0.f)
      pathVertex.dg.Ns = pathVertex.dg.Ng = neg(pathVertex.dg.Ng);

    pathVertex.dg.renderer = &pathContext.context->super;
    pathVertex.dg.P = ray.org + pathState.shadowCatcherDist * ray.dir;
    const float rayConeWidth = rayCone.width
        + rayCone.dwdt * pathState.shadowCatcherDist; // propagate to hit
    const float eps =
        calcEpsilon(pathVertex.dg.P, ray.dir, pathState.shadowCatcherDist);
    pathVertex.dg.P = pathVertex.dg.P + eps * pathVertex.dg.Ng;

    vec3f unshaded = make_vec3f(0.f); // illumination without occluders
    vec3f shaded = make_vec3f(0.f); // illumination including shadows
    const World *uniform world = pathContext.world;
    const uniform PathTracerData &pathtracerData =
        *((const uniform PathTracerData *)world->pathtracerData);
    // TODO use lightsSamples
    for (uniform uint32 i = 0; i < pathContext.numLights; i++) {
      const Light *uniform light = pathtracerData.lights[i];
      float dummy;
      const vec2f s =
          LDSampler_get3LightSamples(pathState.ldSampler, i, true, dummy);
      Light_SampleRes ls =
          Light_dispatch_sample(light, pathVertex.dg, s, pathState.time, ffh);

      // skip when zero contribution from light
      if (reduce_max(ls.weight) <= 0.0f | ls.pdf <= PDF_CULLING)
        continue;

      // evaluate a white diffuse BRDF
      const float brdf =
          clamp(dot(ls.dir, pathVertex.dg.Ns)); // * one_over_pi cancels anyway

      // skip when zero contribution from material
      if (brdf <= 0.0f)
        continue;

      // test for shadows
      Ray shadowRay;
      setRay(shadowRay, pathVertex.dg.P, ls.dir, 0.0f, ls.dist, pathState.time);
      // connect ray cone to point on light
      RayCone shadowCone;
      shadowCone.width = rayConeWidth;
      shadowCone.dwdt = -rayConeWidth * rcp(ls.dist);

      // Trace ray in clipping geometries scene, fill array with ray intervals
      RayIntervals rayIntervals;
      traceClippingRay(pathContext.world, shadowRay, rayIntervals, ffh);

      const vec3f unshadedLightContrib = pathState.throughput * ls.weight
          * brdf; // * misHeuristic(pathContext, ls.pdf, brdf);
      unshaded = unshaded + unshadedLightContrib;
      shaded = shaded
          + transparentShadow(pathContext.context,
              pathContext.world,
              unshadedLightContrib,
              shadowRay,
              shadowCone,
              rayIntervals,
              pathState.currentMedium,
              ffh);
    }
    // order of args important to filter NaNs (in case unshaded.X is zero)
    const vec3f ratio = min(
        pathState.throughput * shaded * rcp(unshaded), pathState.throughput);

    // alpha blend-in black shadow
    pathState.throughput = ratio;
    pathState.contribution = make_vec3f(0.f);

    return true;
  }

  // update dist for potential next intersection (if transparent)
  pathState.shadowCatcherDist -= ray.t;
  return false;
}

OSPRAY_END_ISPC_NAMESPACE
