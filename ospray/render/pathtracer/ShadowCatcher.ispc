// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/pathtracer/PathStructs.ih"
#include "render/pathtracer/PathTracerDefines.ih"
#include "render/pathtracer/ShadowCatcher.ih"
#include "render/pathtracer/TransparentShadow.ih"

#include "render/Renderer.ih"

#include "common/Ray.ih"

bool shadowCatcher(const PathContext &pathContext,
    PathState &pathState,
    PathVertex &pathVertex,
    const Ray &ray,
    ScreenSample &sample)

{
  if (!pathState.straightPath)
    return false;

  // TODO use MIS as well
  // consider real (flagged) geometries with material and move into
  // light loop (will also handle MIS)
  if (pathState.shadowCatcherDist <= ray.t
      && pathState.shadowCatcherDist > ray.t0) {
    // "postIntersect" of shadowCatcher plane
    pathVertex.dg.Ns = pathVertex.dg.Ng =
        make_vec3f(pathContext.context->shadowCatcherPlane);
    if (dot(ray.dir, pathVertex.dg.Ng) >= 0.f)
      pathVertex.dg.Ns = pathVertex.dg.Ng = neg(pathVertex.dg.Ng);
    const float eps =
        calcEpsilon(pathVertex.dg.P, ray.dir, pathState.shadowCatcherDist);
    pathVertex.dg.P = ray.org + pathState.shadowCatcherDist * ray.dir
        + eps * pathVertex.dg.Ng;

    vec3f unshaded = make_vec3f(0.f); // illumination without occluders
    vec3f shaded = make_vec3f(0.f); // illumination including shadows
    for (uniform int i = 0; i < pathContext.numLights; i++) {
      const uniform Light *uniform light = pathContext.context->lights[i];

      const vec2f s = LDSampler_getFloat2(
          pathContext.ldSampler, pathState.sampleDim + 4 + i * 2);
      Light_SampleRes ls = light->sample(light, pathVertex.dg, s);

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

      const vec3f unshadedLightContrib = pathState.throughput * ls.weight
          * brdf; // * misHeuristic(pathState, ls.pdf, brdf);
      unshaded = unshaded + unshadedLightContrib;
      shaded = shaded
          + transparentShadow(pathContext.context,
              pathContext.world,
              unshadedLightContrib,
              shadowRay,
              pathState.currentMedium);
    }
    // order of args important to filter NaNs (in case unshaded.X is zero)
    const vec3f ratio = min(
        pathState.throughput * shaded * rcp(unshaded), pathState.throughput);
#ifdef COLORED_SHADOW_HACK
    const float rm = reduce_min(ratio);
    sample.alpha = 1.0f - rm;
    pathState.contribution = ratio - rm;
#else
    // alpha blend-in black shadow
    sample.alpha = 1.0f - luminance(ratio);
    pathState.contribution = make_vec3f(0.f);
#endif
    return true;
  }

  // update dist for potential next intersection (if transparent)
  pathState.shadowCatcherDist -= ray.t;
  return false;
}
