// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/pathtracer/NextEventEstimation.ih"
#include "render/pathtracer/PathStructs.ih"
#include "render/pathtracer/PathTracerDefines.ih"
#include "render/pathtracer/PathTracerUtil.ih"
#include "render/pathtracer/TransparentShadow.ih"

#ifdef OSPRAY_ENABLE_VOLUMES
#include "render/pathtracer/volumes/VolumeSampler.ih"
#endif

#include "common/RayCone.ih"
#include "common/World.ih"
#include "math/Distribution1D.ih"
#include "math/random.ih"
#include "math/sampling.ih"
// c++ shared
#include "lights/LightDispatch.ih"
#include "lights/LightShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec3f nextEventEstimation(const PathContext &pathContext,
    PathState &pathState,
    PathVertex &pathVertex,
    float rayConeWidth,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlags ff = getFeatureFlags(ffh);

  // direct lighting including shadows and MIS
  vec3f L = make_vec3f(0.f);

  const uint32 numLightSamples = pathState.firstBounceLight
      ? pathContext.numFirstBounceLightSamples
      : pathContext.numIndirectBounceLightSamples;

  // remember for MIS
  pathVertex.numLightSamples = numLightSamples;

  for (uniform uint32 i = 0; i < numLightSamples; i++) {
    // select a random light source from the list
    float ss;
    const vec2f s = LDSampler_get3LightSamples(
        pathState.ldSampler, i, numLightSamples > 1, ss);
    const Sample1D lSelectSample = Distribution1D_sample(
        pathContext.numLights, pathContext.lightsCDF, 0, ss);

    const float lightSelectionProb = lSelectSample.prob * numLightSamples;
    const Light *light = pathContext.lights[lSelectSample.idx];
    // sample the contribution from the random light source
    Light_SampleRes ls;
    foreach_unique (l in light)
      ls = Light_dispatch_sample(l, pathVertex.dg, s, pathState.time, ffh);

    // adjust the contribution with the probabiltiy of selecting the light
    // source
    ls.weight = ls.weight / lightSelectionProb;
    ls.pdf *= lightSelectionProb;

    // skip when zero contribution from light
    if (reduce_max(ls.weight) <= 0.0f | ls.pdf <= PDF_CULLING)
      continue;

    // evaluate BSDF
    Scattering_EvalRes fe;
    if (ff.geometry && pathVertex.type == SURFACE) {
      foreach_unique (f in pathVertex.bsdf)
        if (f != NULL)
          fe = BSDF_dispatch_eval(f, pathVertex.wo, ls.dir, ffh);
    } else {
#ifdef OSPRAY_ENABLE_VOLUMES
      if (ff.other & FFO_VOLUME_IN_SCENE) {
        foreach_unique (v in pathVertex.volume)
          if (v != NULL)
            fe = HenyeyGreenstein_eval(v->anisotropy, pathVertex.wo, ls.dir);
      }
#endif
    }

    // skip when zero contribution from material
    if (reduce_max(fe.value) <= 0.0f)
      continue;

    // test for shadows with rayCones
    Ray shadowRay;
    vec3f org = pathVertex.dg.P;
    if (pathVertex.type != VOLUME) {
      if (dot(pathVertex.dg.Ng, ls.dir) < 0.f)
        org = org - (2.0f * pathVertex.dg.epsilon) * pathVertex.dg.Ng;
    }
    setRay(shadowRay, org, ls.dir, 0.f, ls.dist, pathState.time);
    // connect ray cone to point on light
    RayCone shadowCone;
    shadowCone.width = rayConeWidth;
    shadowCone.dwdt = -rayConeWidth * rcp(ls.dist);

    // Trace ray in clipping geometries scene, fill array with ray intervals
    RayIntervals rayIntervals;
    traceClippingRay(pathContext.world, shadowRay, rayIntervals, ffh);

    const vec3f throughput = pathState.throughput * fe.value;

    const vec3f unshadedLightContrib = throughput * ls.weight;
    const vec3f lightContrib = transparentShadow(pathContext.context,
        pathContext.world,
        unshadedLightContrib,
        shadowRay,
        shadowCone,
        rayIntervals,
        pathState.currentMedium,
        ffh);
    if (reduce_max(lightContrib) > 0) {
#ifdef OSPRAY_ENABLE_VOLUMES
      if (ff.other & FFO_VOLUME_IN_SCENE) {
        const float T = volumeTransmittance(pathContext.world,
            shadowRay,
            rayIntervals,
            &pathState.randomSampler,
            ffh);
        if (reduce_max(T) > 0) {
          // we have to use an independent transmittance estimate for MIS to get
          // a correct result
          const float T_mis =
#ifdef OSPRAY_PATHTRACER_DEBUG
              pathContext.disableFWD || pathContext.disableNEE
              ? 1.f
              :
#endif
              volumeTransmittance(pathContext.world,
                  shadowRay,
                  rayIntervals,
                  &pathState.randomSampler,
                  ffh);
          L = L
              + T * lightContrib
                  * misHeuristic(pathContext, ls.pdf, fe.pdf * T_mis);
        }
      } else
#endif
      {
        L = L + lightContrib * misHeuristic(pathContext, ls.pdf, fe.pdf);
      }
    }
  }

  return L;
}

OSPRAY_END_ISPC_NAMESPACE
