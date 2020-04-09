// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/pathtracer/NextEventEstimation.ih"
#include "render/pathtracer/PathStructs.ih"
#include "render/pathtracer/PathTracerDefines.ih"
#include "render/pathtracer/PathTracerUtil.ih"
#include "render/pathtracer/TransparentShadow.ih"

#include "render/pathtracer/volumes/VolumeSampler.ih"

#include "math/sampling.ih"

vec3f nextEventEstimation(const PathContext &pathContext,
    const PathState &pathState,
    PathVertex &pathVertex)
{
  // direct lighting including shadows and MIS
  vec3f L = make_vec3f(0.f);
  for (uniform int i = 0; i < pathContext.numLights; i++) {
    const uniform Light *uniform light = pathContext.context->lights[i];

    const vec2f s = LDSampler_getFloat2(
        pathContext.ldSampler, pathState.sampleDim + 4 + i * 2);
    Light_SampleRes ls = light->sample(light, pathVertex.dg, s);

    // skip when zero contribution from light
    if (reduce_max(ls.weight) <= 0.0f | ls.pdf <= PDF_CULLING)
      continue;

    // evaluate BSDF
    Scattering_EvalRes fe;
    if (pathVertex.type == SURFACE) {
      foreach_unique (f in pathVertex.bsdf) {
        if (f != NULL)
          fe = f->eval(f, pathVertex.wo, ls.dir);
      }
    } else {
      foreach_unique (v in pathVertex.volume) {
        if (v != NULL)
          fe = HenyeyGreenstein_eval(v->anisotropy, pathVertex.wo, ls.dir);
      }
    }

    // skip when zero contribution from material
    if (reduce_max(fe.value) <= 0.0f)
      continue;

    // test for shadows
    Ray shadowRay;
    vec3f org = pathVertex.dg.P;
    if (pathVertex.type != VOLUME) {
      if (dot(pathVertex.dg.Ng, ls.dir) < 0.f)
        org = org - (2.0f * pathVertex.dg.epsilon) * pathVertex.dg.Ng;
    }
    setRay(shadowRay, org, ls.dir, 0.f, ls.dist, pathState.time);

    // Trace ray in clipping geometries scene, fill array with ray intervals
    RayIntervals rayIntervals;
    traceClippingRay(pathContext.world, shadowRay, rayIntervals);

    const vec3f throughput = pathState.throughput * fe.value;

#if ROULETTE == 1
    // Russian roulette adjustment
    if (pathState.depth >= pathContext.context->rouletteDepth) {
      const float contProb =
          min(luminance(throughput * rcp(fe.pdf)), MAX_ROULETTE_CONT_PROB);
      fe.pdf *= contProb;
    }
#endif

    const vec3f unshadedLightContrib = throughput * ls.weight;
    const vec3f lightContrib = transparentShadow(pathContext.context,
        pathContext.world,
        unshadedLightContrib,
        shadowRay,
        rayIntervals,
        pathState.currentMedium);
    if (reduce_max(lightContrib) > 0) {
      const float T = volumeTransmittance(pathState,
          pathContext.world,
          shadowRay,
          rayIntervals,
          pathContext.ldSampler,
          pathContext.randomSampler);
      if (reduce_max(T) > 0) {
        // we have to use an independent transmittance estimate for MIS to get a
        // correct result
        const float T_mis = pathState.disableFWD || pathState.disableNEE
            ? 1.f
            : volumeTransmittance(pathState,
                pathContext.world,
                shadowRay,
                rayIntervals,
                pathContext.ldSampler,
                pathContext.randomSampler);
        L = L
            + T * lightContrib
                * misHeuristic(pathState, ls.pdf, fe.pdf * T_mis);
      }
    }
  }
  return L;
}
