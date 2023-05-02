// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SciVis.ih"
#include "camera/Camera.ih"
#include "camera/CameraDispatch.ih"
#include "common/FeatureFlagsEnum.h"
#include "common/World.ih"
#include "fb/FrameBuffer.ih"
#include "fb/FrameBufferDispatch.ih"
#include "fb/RenderTaskDesc.ih"
#include "math/random.ih"
#include "math/sampling.ih"
#include "pf/PixelFilterDispatch.ih"
#include "surfaces.ih"
#include "volume/VolumetricModel.ih"
#include "volumes.ih"
// c++ shared
#include "SciVisShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

static void SciVis_renderSample(Renderer *uniform _self,
    FrameBuffer *uniform fb,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  uniform SciVis *uniform self = (uniform SciVis * uniform) _self;

  const uniform FeatureFlags ff = getFeatureFlags(ffh);

  LDSampler ldSamplerObj;
  varying LDSampler *uniform ldSampler = &ldSamplerObj;
  LDSampler_init(ldSampler,
      self->super.mathConstants,
      sample.sampleID.x + fb->size.x * sample.sampleID.y,
      sample.sampleID.z);

  uniform bool firstHit = true;
  bool allHitsFullyTransmissive = true;
  const float originalRayTFar = sample.ray.t;

#ifdef OSPRAY_ENABLE_VOLUMES
#ifdef OSPRAY_TARGET_SYCL
  // We only support a single volume interval on the GPU
  VolumeInterval vInterval;
  VolumeIntervals volumeIntervals;
  volumeIntervals.numVolumeIntervals = 0;
  volumeIntervals.numAllocated = 1;
  volumeIntervals.intervals = &vInterval;
#else
  // Allocate memory for volume intervals
  VolumeIntervals volumeIntervals;
  allocVolumeIntervals(volumeIntervals);
#endif
#endif

  // This is our main ray
  Ray &ray = sample.ray;

  // First trace the ray across clipping scene to calculate ray intervals,
  // this step should keep ray structure unchanged
  RayIntervals rayIntervals;
  traceClippingRay(world, ray, rayIntervals, ffh);

  // Iterate over all translucent geometry till we are fully opaque
  vec3f outColor = make_vec3f(0.f);
  vec3f outTransmission = make_vec3f(1.f);
  while (true) {
    if (ff.geometry) {
      // Then trace normal geometry using calculated ray intervals,
      // if hit ray.t will be updated
      traceGeometryRayIntervals(world, ray, rayIntervals, ffh);
    }

#ifdef OSPRAY_ENABLE_VOLUMES
    Ray volumeRay = ray;
    if (ff.other & FFO_VOLUME_IN_SCENE) {
      // Determine volume intervals by tracing ray in the volume scene
      traceVolumeRay(world, volumeRay, volumeIntervals);

      // Sample volumes across volume intervals (in front of geometry hit)
      if (volumeIntervals.numVolumeIntervals > 0) {
        SciVisRenderContext rc;
        rc.renderer = self;
        rc.fb = fb;
        rc.world = world;
        rc.sample = sample;
        rc.ldSampler = ldSampler;
        volumeRay.geomID = RTC_INVALID_GEOMETRY_ID;
        vec4f volumeColor = integrateVolumeIntervalsGradient(rc,
            volumeIntervals,
            rayIntervals,
            volumeRay,
            ldSampler,
            self->volumeSamplingRate,
            ffh);

        // Blend volume
        outColor = outColor + outTransmission * make_vec3f(volumeColor);
        outTransmission = outTransmission * volumeColor.w;

        if (hadHit(volumeRay) && sample.geomID == RTC_INVALID_GEOMETRY_ID) {
          sample.primID = 0;
#ifdef OSPRAY_TARGET_SYCL
          {
            const uint32 instID = volumeRay.instID;
#else
          foreach_unique (instID in volumeRay.instID) {
#endif
            if (instID != RTC_INVALID_GEOMETRY_ID) {
              const Instance *uniform instance = *(world->instances + instID);
              sample.instID = (instance->userID == RTC_INVALID_GEOMETRY_ID)
                  ? instID
                  : instance->userID;
              const VolumetricModel *volms =
                  *(instance->group->volumetricModels + volumeRay.geomID);

#ifdef OSPRAY_TARGET_SYCL
              {
                const VolumetricModel *uniform volm = volms;
#else
              foreach_unique (volm in volms) {
#endif
                sample.geomID = (volm->userID == RTC_INVALID_GEOMETRY_ID)
                    ? volumeRay.geomID
                    : volm->userID;
              }
            }
          }
        }
      }
    }
#endif

    // Add contribution from visible lights, P is used by light
    // evaluation
    DifferentialGeometry dg;
    if (self->visibleLights) {
      dg.P = ray.org;
      outColor =
          outColor + outTransmission * evaluateLights(world, dg, ray, ffh);
    }

    // If any geometry has been hit
    const bool rayHadHit = hadHit(ray);
    if (ff.geometry && rayHadHit) {
      // Prepare differential geometry structure
      postIntersect(world,
          &self->super,
          dg,
          ray,
          DG_NG | DG_NS | DG_NORMALIZE | DG_FACEFORWARD | DG_COLOR
              | DG_TEXCOORD,
          ffh);

      // Shade geometry
      SSI surfaceShading;
      surfaceShading = SciVis_computeShading(
          self, world, dg, sample, ldSampler, ray.dir, ffh);

      // Initialize other per sample data with first hit values
      // In addition to considering the first hit, all previous, fully
      // transmissive hits will lead the normal/albedo to be replaced by the
      // current hit
      if (firstHit || allHitsFullyTransmissive) {
#ifdef OSPRAY_ENABLE_VOLUMES
        sample.z = min(ray.t, volumeRay.t);
#endif
        sample.albedo = surfaceShading.albedo;
        sample.normal = dg.Ns;
        if (sample.geomID == RTC_INVALID_GEOMETRY_ID) {
          sample.instID = dg.instID;
          sample.geomID = dg.objID;
          sample.primID = ray.primID;
        }
        firstHit = false;
      }

      // A sequence of hits is considered fully transparent if it's total
      // normalized transmission is close to 1.0f and all previous hits were
      // fully transmissive
      allHitsFullyTransmissive = allHitsFullyTransmissive
          && surfaceShading.opacity < self->super.minContribution;

      // Blend with output final color
      outColor = outColor + outTransmission * surfaceShading.shadedColor;
      outTransmission = outTransmission * surfaceShading.transmission;

      // Early exit if remaining transmission is below min contribution
      // threshold
      if (luminance(outTransmission) < self->super.minContribution) {
        outTransmission = make_vec3f(0.f);
        break;
      }

      // Prepare ray for next loop iteration,
      // start from the last geometry hit all over to initial Tfar
      setRay(ray, ray.t + dg.epsilon, originalRayTFar);
    } else {
      // Blend background with output final color
      vec4f backgroundColor =
          Renderer_getBackground(&self->super, sample.pos, ffh);
      outColor = outColor + outTransmission * make_vec3f(backgroundColor);
      outTransmission = outTransmission * (1.f - backgroundColor.w);

      // Initialize other per sample data with first hit values
      // If the ray hits the background and comes from a fully transmissive
      // path, we overwrite normals/albedo by background values
      if (firstHit || allHitsFullyTransmissive) {
#ifdef OSPRAY_ENABLE_VOLUMES
        sample.z = min(ray.t, volumeRay.t);
#endif
        sample.albedo = make_vec3f(backgroundColor); // TODO volume albedo
        sample.normal = make_vec3f(0.f);
      }

      // No more intersections
      break;
    }
  }

#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
  // Only a single interval is supported on the GPU, we don't allocate
  // dynamically
  freeVolumeIntervals(volumeIntervals);
#endif
  sample.rgb = outColor;
  sample.alpha = 1.f - luminance(outTransmission);
}

SYCL_EXTERNAL vec3f SciVis_computeAO(const uniform SciVis *uniform self,
    const World *uniform world,
    varying LDSampler *uniform ldSampler,
    const varying DifferentialGeometry &dg,
    const uniform int sampleCnt,
    const uniform float aoRadius,
    const varying vec3i &sampleID,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform int accumID = reduce_max(sampleID.z) * sampleCnt;

  // init TEA RNG //
  RandomTEA rng_state;
  varying RandomTEA *const uniform rng = &rng_state;
  RandomTEA_Constructor(rng, sampleID.x, sampleID.y);
  const vec2f rot = RandomTEA_getFloats(rng);

  vec3f hits = make_vec3f(0.f);

  const linear3f localToWorld = frame(dg.Ns);

  for (uniform int i = 0; i < sampleCnt; i++) {
    // Use base-5 and base-7 Halton to avoid correlation with sample position
    const vec2f halton =
        make_vec2f(PermHalton_sample5(self->super.mathConstants, accumID + i),
            PermHalton_sample7(self->super.mathConstants, accumID + i));
    const vec2f r = CranleyPattersonRotation(halton, rot);
    const vec3f local_ao_dir = cosineSampleHemisphere(r);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    if (dot(ao_dir, dg.Ns) < 0.01f) { // check below surface
      hits = hits + 1.f;
      continue;
    }

    Ray ao_ray;
    setRay(ao_ray, dg.P, ao_dir, dg.epsilon, aoRadius);

    hits = hits
        + (1.f
            - lightAlpha(
                self, ao_ray, world, ldSampler, make_vec3f(1.f), 0.1f, ffh));
  }

  // the cosTheta of cosineSampleHemispherePDF and dot(shadingNormal, ao_dir)
  // cancel
  return 1.0f - (hits / (float)sampleCnt);
}

#define renderSampleFn SciVis_renderSample
#include "render/RendererRenderTaskFn.inl"
#undef renderSampleFn

#ifdef OSPRAY_TARGET_SYCL
SYCL_EXTERNAL void SciVis_renderTask(Renderer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    World *uniform world,
    const uint32 *uniform taskIDs,
    const int taskIndex0,
    const uniform FeatureFlagsHandler &ffh)
{
  Renderer_default_renderTask(
      self, fb, camera, world, taskIDs, taskIndex0, ffh);
}
#else
export void SciVis_renderTasks(void *uniform _self,
    void *uniform _fb,
    void *uniform _camera,
    void *uniform _world,
    void *uniform _taskIDs,
    uniform uint32 numTasks)
{
  Renderer *uniform self = (Renderer * uniform) _self;
  FrameBuffer *uniform fb = (FrameBuffer * uniform) _fb;
  Camera *uniform camera = (Camera * uniform) _camera;
  World *uniform world = (World * uniform) _world;
  const uint32 *uniform taskIDs = (const uint32 *uniform)_taskIDs;
  uniform FeatureFlagsHandler ffh;
  launch[numTasks] Renderer_default_renderTask(
      self, fb, camera, world, taskIDs, ffh);
  sync;
}
#endif

OSPRAY_END_ISPC_NAMESPACE
