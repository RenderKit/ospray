// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/pathtracer/GeometryLight.ih"
#include "render/pathtracer/NextEventEstimation.ih"
#include "render/pathtracer/PathSampler.ih"
#include "render/pathtracer/PathStructs.ih"
#include "render/pathtracer/PathTracerDefines.ih"
#include "render/pathtracer/PathTracerUtil.ih"
#include "render/pathtracer/ShadowCatcher.ih"
#include "render/pathtracer/TransparentShadow.ih"
#include "render/pathtracer/VirtualLight.ih"

#include "common/Intersect.ih"
#include "common/World.ih"
#include "math/random.ih"
#include "render/Material.ih"
#include "render/MaterialDispatch.ih"
#include "render/bsdfs/MicrofacetAlbedoTables.ih"
#include "render/bsdfs/ShadingContext.ih"
#include "render/materials/Medium.ih"
#ifdef OSPRAY_ENABLE_VOLUMES
#include "render/pathtracer/volumes/VolumeSampler.ih"
#endif

// c++ shared
#include "PathTracerDataShared.h"
#include "PathTracerShared.h"
#include "common/RayCone.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

inline void postIntersect(const PathContext &pathContext,
    PathVertex &pathVertex,
    Ray &ray,
    RayCone &rayCone,
    const uniform FeatureFlagsHandler &ffh)
{
  const PathTracer *uniform pt = pathContext.context;
  const uniform FeatureFlags ff = getFeatureFlags(ffh);
  if (ff.geometry && pathVertex.type == SURFACE) {
    postIntersect(pathContext.world,
        &pt->super,
        pathVertex.dg,
        ray,
        rayCone,
        DG_NS | DG_NG | DG_FACEFORWARD | DG_NORMALIZE | DG_TEXCOORD | DG_COLOR
            | DG_TANGENTS | DG_MOTIONBLUR,
        ffh);
  }
#ifdef OSPRAY_ENABLE_VOLUMES
  if ((ff.other & FFO_VOLUME_IN_SCENE) && pathVertex.type == VOLUME) {
    pathVertex.dg.P = ray.org + ray.t * ray.dir;
    pathVertex.dg.renderer = &pt->super;

    pathVertex.dg.instID = ray.instID;
    foreach_unique (instID in ray.instID) {
      if (instID != RTC_INVALID_GEOMETRY_ID) {
        const World *uniform world = pathContext.world;
        const Instance *uniform instance = *(world->instances + instID);
        if (instance->userID != RTC_INVALID_GEOMETRY_ID)
          pathVertex.dg.instID = instance->userID;
      }
    }

    foreach_unique (volm in pathVertex.volume)
      VolumetricModel_postIntersect(volm, pathVertex.dg, ray, 0);
  }
#endif
}

inline Scattering_SampleRes sampleDirection(const vec2f &s,
    const float ss,
    PathVertex &pathVertex,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlags ff = getFeatureFlags(ffh);

  Scattering_SampleRes fs;
  fs.weight = make_vec3f(0.0f);
  if (ff.geometry && pathVertex.type == SURFACE) {
    foreach_unique (f in pathVertex.bsdf) {
      if (f != NULL) {
        fs = BSDF_dispatch_sample(f, pathVertex.wo, s, ss, ffh);
        pathVertex.wi = fs.wi;
        pathVertex.pdf_w = fs.pdf;
      }
    }
    // check consistent side of surface to prevent light leaks
    const bool sameSide = dot(fs.wi, pathVertex.dg.Ng) >= 0.f;
    const bool reflection = fs.type & SCATTERING_REFLECTION;
    if (sameSide != reflection)
      fs.weight = make_vec3f(0.0f);
  }
#ifdef OSPRAY_ENABLE_VOLUMES
  if ((ff.other & FFO_VOLUME_IN_SCENE) && pathVertex.type == VOLUME) {
    foreach_unique (v in pathVertex.volume) {
      if (v != NULL) {
        fs = HenyeyGreenstein_sample(v->anisotropy, pathVertex.wo, s, ss);
        pathVertex.wi = fs.wi;
        pathVertex.pdf_w = fs.pdf;
      }
    }
  }
#endif
  return fs;
}

SYCL_EXTERNAL void samplePath(const PathContext &pathContext,
    PathState &pathState,
    Ray &ray,
    RayCone &rayCone,
    ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  PathVertex lastVertex;
  lastVertex.type = CAMERA;
#ifdef OSPRAY_ENABLE_VOLUMES
  lastVertex.volume = NULL;
#endif
  lastVertex.pdf_w =
      inf; // probability density of previous sampled BSDF, for MIS
  lastVertex.dg.P = ray.org; // P and N also used by light eval
  lastVertex.dg.epsilon = calcEpsilon(ray.org, 0.f);
  lastVertex.dg.Ns = ray.dir;
  lastVertex.dg.Ng = ray.dir;
  lastVertex.numLightSamples = 0;

  uniform ShadingContext ctx;
  ShadingContext_Constructor(&ctx);
  const uniform FeatureFlags ff = getFeatureFlags(ffh);

  if (pathContext.context->shadowCatcher) {
    const Hit hit = intersectPlane(
        ray.org, ray.dir, pathContext.context->shadowCatcherPlane);
    if (hit.hit)
      pathState.shadowCatcherDist = hit.t;
  }
  do {
    if (pathState.shadowCatcherDist
        > ray.t0) // valid hit can hide other geometry
      ray.t = min(pathState.shadowCatcherDist, ray.t);

    // Trace ray in clipping geometries scene, fill array with ray intervals
    RayIntervals rayIntervals;
    traceClippingRay(pathContext.world, ray, rayIntervals, ffh);

    if (ff.geometry) {
      // Trace ray intervals in geometry
      traceGeometryRayIntervals(pathContext.world, ray, rayIntervals, ffh);
    }

    PathVertex pathVertex;
    pathVertex.bsdf = NULL;
    pathVertex.pdf_w = inf;
#ifdef OSPRAY_ENABLE_VOLUMES
    pathVertex.volume = NULL;
#endif
    pathVertex.type = noHit(ray) || !ff.geometry ? ENVIRONMENT : SURFACE;

    if (shadowCatcher(pathContext, pathState, pathVertex, ray, rayCone, ffh)) {
      pathVertex.type = ENVIRONMENT;
    }

    pathVertex.wo = neg(ray.dir);

#ifdef OSPRAY_ENABLE_VOLUMES
    if (ff.other & FFO_VOLUME_IN_SCENE) {
      float extinctionCoefficient;
      float freePath = volumeSampleFreePath(pathContext.world,
          ray,
          rayIntervals,
          &pathState.randomSampler,
          &pathVertex.volume,
          extinctionCoefficient,
          pathVertex.albedo,
          ffh);
      if (freePath < inf) {
        pathVertex.type = VOLUME;
        pathState.throughput = pathState.throughput * pathVertex.albedo;
      }
    }
#endif

    // record depth of primary rays
    if (pathState.depth == 0)
      sample.z = ray.t;

    // background handling
    if (pathVertex.type == ENVIRONMENT
        && (pathContext.context->backgroundRefraction
                ? pathState.specularTransmissionPath
                : pathState.straightPath)) {
      vec4f bg = Renderer_getBackground(
          &pathContext.context->super, pathContext.screen, ffh);
      pathState.contribution =
          pathState.contribution + pathState.throughput * make_vec3f(bg);
      sample.alpha = 1.0f - luminance(pathState.throughput);
      sample.alpha += (1.f - sample.alpha) * bg.w;
    }

#ifdef OSPRAY_PATHTRACER_DEBUG
    if (!pathContext.disableFWD || pathState.depth == 0)
#endif
#ifdef OSPRAY_ENABLE_VOLUMES
      if (pathVertex.type != VOLUME)
#endif
      {
        pathState.contribution = pathState.contribution
            + evaluateVirtualLights(
                pathContext, pathState, lastVertex, pathVertex, ray, ffh);
      }

    if (pathVertex.type == ENVIRONMENT) {
      break;
    }

    // terminate after evaluation of lights and before next shading to always
    // have both samples for MIS except if we have geometry lights (which we
    // still need to evaluate for MIS)
    if (pathState.depth >= pathContext.context->super.maxDepth
        && pathContext.numGeoLights == 0) {
      break;
    }

    postIntersect(pathContext, pathVertex, ray, rayCone, ffh);

#ifdef OSPRAY_PATHTRACER_DEBUG
    if (!pathContext.disableFWD || pathState.depth == 0)
#endif
#ifdef OSPRAY_ENABLE_VOLUMES
      if (pathVertex.type != VOLUME)
#endif
      {
        pathState.contribution = pathState.contribution
            + evaluateGeometryLights(
                pathContext, pathState, lastVertex, pathVertex, ray, ffh);
      }

    // record IDs of primary rays
    if (pathState.depth == 0) {
      sample.primID = pathVertex.dg.primID;
      sample.geomID = pathVertex.dg.objID;
      sample.instID = pathVertex.dg.instID;
    }

    // terminate after evaluation of lights and before next shading to always
    // have both samples for MIS
    if (pathState.depth >= pathContext.context->super.maxDepth
        || pathState.scatteringEvents
            >= pathContext.context->maxScatteringEvents) {
      break;
    }

    // shade surface
    ShadingContext_Constructor(&ctx);
    if (ff.geometry && pathVertex.type == SURFACE) {
      Material *material = (Material *)pathVertex.dg.material;
      foreach_unique (m in material) {
        if (m != NULL) {
          pathVertex.bsdf = Material_dispatch_getBSDF(
              m, &ctx, pathVertex.dg, ray, pathState.currentMedium, ffh);
          if (pathVertex.bsdf != NULL) {
            pathVertex.albedo = pathVertex.bsdf->albedo;
          }
        }
      }
      // terminate path when we don't have any BSDF
      if (pathVertex.bsdf == NULL) {
        break;
      }
    }

    // unconditionally advance sampler state to stay in lockstep across paths
    LDSampler_nextGroup(pathState.ldSampler);
    // next event estimation
    if (isSmooth(pathVertex)) {
      // record last vertex of a specular-only path
      if (pathState.scatteringEvents == 0)
        updateAuxilliaryData(pathState, pathVertex, sample);
#ifdef OSPRAY_PATHTRACER_DEBUG
      if (!pathContext.disableNEE)
#endif
      {
        pathState.contribution = pathState.contribution
            + nextEventEstimation(
                pathContext, pathState, pathVertex, sample.rayCone.width, ffh);
        pathState.firstBounceLight = false;
      }
    }

    float ss;
    uint32 uu3;
    const vec2f s = LDSampler_getNext4Samples(pathState.ldSampler, ss, uu3);
    Scattering_SampleRes fs = sampleDirection(s, ss, pathVertex, ffh);

    // terminate path when zero contribution from material
    if (reduce_max(fs.weight) <= 0.0f || fs.pdf <= PDF_CULLING) {
      break;
    }

    pathState.throughput = pathState.throughput * fs.weight;

    // Russian roulette
    if (pathState.depth >= pathContext.context->rouletteDepth) {
      const float rr = LDSampler_finalizeDim3(pathState.ldSampler, uu3);
      const float contProb =
          min(reduce_max(pathState.throughput), MAX_ROULETTE_CONT_PROB);
      if (rr > contProb) {
        break;
      }
      pathState.throughput = pathState.throughput * rcp(contProb);
    }

    // compute attenuation with Beer's law
    if (reduce_min(pathState.currentMedium.attenuation) < 0.f)
      pathState.throughput = pathState.throughput
          * expf(pathState.currentMedium.attenuation * ray.t);

    vec3f ray_org = pathVertex.dg.P;
    if (ff.geometry && pathVertex.type == SURFACE) {
      // update currentMedium if we hit a medium interface
      // TODO: support nested dielectrics
      if (fs.type & SCATTERING_TRANSMISSION) {
        ray_org = ray_org - (2.0f * pathVertex.dg.epsilon) * pathVertex.dg.Ng;
        Material *material = (Material *)pathVertex.dg.material;
        foreach_unique (m in material) {
          if (m != NULL) {
            Material_dispatch_selectNextMedium(
                m, pathVertex.dg, pathState.currentMedium, ffh);
          }
        }
      }
    }

    // keep lastBsdfPdf and lastDg when there was a specular transmission
    // to better combine MIS with transparent shadows
    if (fs.type & ~SCATTERING_SPECULAR_TRANSMISSION
        || pathVertex.type == VOLUME) {
      lastVertex = pathVertex;
    }

    // continue the path
    if (!eq(ray.dir, fs.wi))
      pathState.straightPath = false;
    if (!(fs.type & SCATTERING_SPECULAR_TRANSMISSION))
      pathState.specularTransmissionPath = false;

    setRay(ray, ray_org, fs.wi, pathState.time);
    pathState.depth++;
    if (fs.type & SCATTERING_SMOOTH)
      pathState.scatteringEvents++;
  } while (reduce_max(pathState.throughput)
      > pathContext.context->super.minContribution);

  sample.rgb = pathState.contribution;
  if (isnan(pathState.contribution.x) || isnan(pathState.contribution.y)
      || isnan(pathState.contribution.z)) {
    sample.rgb = make_vec3f(0.f);
    sample.alpha = 1.0f;
  }
}

OSPRAY_END_ISPC_NAMESPACE
