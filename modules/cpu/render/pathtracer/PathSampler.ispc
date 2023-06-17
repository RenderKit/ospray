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

OSPRAY_BEGIN_ISPC_NAMESPACE

inline void postIntersect(const PathContext &pathContext,
    const PathState &,
    PathVertex &pathVertex,
    Ray &ray,
    const uniform FeatureFlagsHandler &ffh)
{
  const PathTracer *uniform pt = pathContext.context;
  const uniform FeatureFlags ff = getFeatureFlags(ffh);
  if (ff.geometry && pathVertex.type == SURFACE) {
    postIntersect(pathContext.world,
        &pt->super,
        pathVertex.dg,
        ray,
        DG_NS | DG_NG | DG_FACEFORWARD | DG_NORMALIZE | DG_TEXCOORD | DG_COLOR
            | DG_TANGENTS | DG_MOTIONBLUR,
        ffh);
  }
#ifdef OSPRAY_ENABLE_VOLUMES
  if ((ff.other & FFO_VOLUME_IN_SCENE) && pathVertex.type == VOLUME) {
    pathVertex.dg.P = ray.org + ray.t * ray.dir;
    pathVertex.dg.renderer = &pt->super;

    pathVertex.dg.instID = ray.instID;
#ifdef OSPRAY_TARGET_SYCL
    {
      const uint32 instID = ray.instID;
#else
    foreach_unique (instID in ray.instID) {
#endif
      if (instID != RTC_INVALID_GEOMETRY_ID) {
        const World *uniform world = pathContext.world;
        const Instance *uniform instance = *(world->instances + instID);
        if (instance->userID != RTC_INVALID_GEOMETRY_ID)
          pathVertex.dg.instID = instance->userID;
      }
    }

#ifdef OSPRAY_TARGET_SYCL
    {
      const VolumetricModel *volm = pathVertex.volume;
#else
    foreach_unique (volm in pathVertex.volume) {
#endif
      VolumetricModel_postIntersect(volm, pathVertex.dg, ray, 0);
    }
  }
#endif
}

inline Scattering_SampleRes sampleDirection(const PathContext &pathContext,
    const PathState &pathState,
    PathVertex &pathVertex,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlags ff = getFeatureFlags(ffh);

  const vec2f ss =
      LDSampler_getFloat2(pathContext.ldSampler, pathState.sampleDim);
  const float s =
      LDSampler_getFloat(pathContext.ldSampler, pathState.sampleDim + 2);
  Scattering_SampleRes fs;
  if (ff.geometry && pathVertex.type == SURFACE) {
#ifdef OSPRAY_TARGET_SYCL
    {
      const BSDF *f = pathVertex.bsdf;
#else
    foreach_unique (f in pathVertex.bsdf) {
#endif
      if (f != NULL) {
        fs = BSDF_dispatch_sample(f, pathVertex.wo, ss, s, ffh);
        pathVertex.wi = fs.wi;
        pathVertex.pdf_w = fs.pdf;
      }
    }
  }
#ifdef OSPRAY_ENABLE_VOLUMES
  if ((ff.other & FFO_VOLUME_IN_SCENE) && pathVertex.type == VOLUME) {
#ifdef OSPRAY_TARGET_SYCL
    {
      const VolumetricModel *v = pathVertex.volume;
#else
    foreach_unique (v in pathVertex.volume) {
#endif
      if (v != NULL) {
        fs = HenyeyGreenstein_sample(v->anisotropy, pathVertex.wo, ss, s);
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
    if (noHit(ray) || !ff.geometry) {
      pathVertex.type = ENVIRONMENT;
    } else {
      pathVertex.type = SURFACE;
    }

    if (shadowCatcher(pathContext, pathState, pathVertex, ray, sample, ffh)) {
      pathVertex.type = ENVIRONMENT;
    }

    pathVertex.wo = neg(ray.dir);

#ifdef OSPRAY_ENABLE_VOLUMES
    if (ff.other & FFO_VOLUME_IN_SCENE) {
      float extinctionCoefficient;
      float freePath = volumeSampleFreePath(pathContext.world,
          ray,
          rayIntervals,
          pathContext.randomSampler,
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
          &pathContext.context->super, *pathContext.pixel, ffh);
      pathState.contribution =
          pathState.contribution + pathState.throughput * make_vec3f(bg);
      sample.alpha = 1.0f - luminance(pathState.throughput);
      sample.alpha += (1.f - sample.alpha) * bg.w;
    }

    if ((!pathState.disableFWD || pathState.depth == 0)
        && pathVertex.type != VOLUME) {
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
    const World *uniform w = pathContext.world;
    const uniform PathTracerData &pathtracerData =
        *((const uniform PathTracerData *)w->pathtracerData);
    if (pathState.depth >= pathContext.context->super.maxDepth
        && pathtracerData.numGeoLights == 0) {
      break;
    }

    postIntersect(pathContext, pathState, pathVertex, ray, ffh);

    if (!pathState.disableFWD && (pathVertex.type != VOLUME)
        && (ff.other & FFO_LIGHT_GEOMETRY)) {
      pathState.contribution = pathState.contribution
          + evaluateGeometryLights(
              pathContext, pathState, lastVertex, pathVertex, ray);
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
#ifdef OSPRAY_TARGET_SYCL
      {
        const Material *m = material;
#else
      foreach_unique (m in material) {
#endif
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

    // next event estimation
    if (!pathState.disableNEE && isSmooth(pathVertex)) {
      pathState.contribution = pathState.contribution
          + nextEventEstimation(pathContext, pathState, pathVertex, ffh);
    }

    Scattering_SampleRes fs =
        sampleDirection(pathContext, pathState, pathVertex, ffh);

    if (pathState.auxFree && (fs.type & SCATTERING_SMOOTH)) {
      updateAuxilliaryData(pathState, pathVertex, sample);
    }

    // terminate path when zero contribution from material
    if (reduce_max(fs.weight) <= 0.0f || fs.pdf <= PDF_CULLING) {
      break;
    }

    pathState.throughput = pathState.throughput * fs.weight;

#if ROULETTE == 1
    // Russian roulette
    if (pathState.depth >= pathContext.context->rouletteDepth) {
      const float rr = RandomSampler_getFloat(pathContext.randomSampler);
      const float contProb =
          min(luminance(pathState.throughput), MAX_ROULETTE_CONT_PROB);
      if (rr > contProb) {
        break;
      }
      pathState.throughput = pathState.throughput * rcp(contProb);
      fs.pdf *= contProb;
    }
#endif

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
#ifdef OSPRAY_TARGET_SYCL
        {
          const Material *m = material;
#else
        foreach_unique (m in material) {
#endif
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
    pathState.sampleDim += pathContext.numBounceSampleDims;
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
