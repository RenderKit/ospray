// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/box.ih"

#include "camera/Camera.ih"
#include "camera/CameraDispatch.ih"
#include "common/FeatureFlags.ih"
#include "common/Instance.ih"
#include "common/Intersect.ih"
#include "common/VolumeIntervals.ih"
#include "common/World.ih"
#include "fb/FrameBufferDispatch.ih"
#include "fb/RenderTaskDesc.ih"
#include "math/random.ih"
#include "math/sampling.ih"
#include "render/Renderer.ih"
#include "render/ScreenSample.ih"
#include "render/materials/OBJ.ih"
#include "render/scivis/surfaces.ih"
#include "render/scivis/volumes.ih"
#include "render/util.ih"
#include "texture/Texture2D.ih"
#include "volume/VolumetricModel.ih"
#ifdef OSPRAY_ENABLE_VOLUMES
#include "volume/Volume.ih"
#include "volume/VolumetricModel.ih"
#include "volume/transferFunction/LinearTransferFunction.ih"
#include "volume/transferFunction/TransferFunctionDispatch.ih"
#endif
// c++ shared
#include "DistributedRaycastShared.h"
#include "common/DistributedWorldShared.h"
#include "fb/FrameBufferShared.h"
#include "fb/SparseFBShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

#ifdef OSPRAY_ENABLE_VOLUMES
// The distributed raycast renderer uses its own volume interval integration
// because we want to apply the jitter before offsetting our step size to stay
// inside the region, not after.
inline vec4f DRR_integrateVolumeInterval(const ScreenSample &sample,
    const VolumeInterval &interval,
    Ray &ray,
    uniform float samplingRate,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlags ff = getFeatureFlags(ffh);

  VolumetricModel *varying volModel = interval.volumetricModel;
  // Note: required to WA compiler dropping symbols on link incorrectly
  if (volModel == NULL) {
    LinearTransferFunction_get(NULL, 0.f);
  }

  Ray transformedRay = ray;
  AffineSpace3f instanceRcpXfm = interval.instance->rcp_xfm;
  transformRay(transformedRay, instanceRcpXfm);

  uniform uint8 intervalIteratorBuffer[VKL_MAX_INTERVAL_ITERATOR_SIZE];

  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;
#ifdef OSPRAY_TARGET_SYCL
  {
    VolumetricModel *uniform vm = volModel;
#else
  foreach_unique (vm in volModel) {
#endif
    Volume *uniform volume = vm->volume;
    TransferFunction *uniform tf = vm->transferFunction;

    float time = 0.5f;

#if OPENVKL_VERSION_MAJOR == 1
    // We know if we have OpenVKL version 1 we're not targetting SYCL
    VKLIntervalIterator intervalIterator =
        vklInitIntervalIteratorV(vm->vklIntervalContext,
            (varying vkl_vec3f *)&ray.org,
            (varying vkl_vec3f *)&ray.dir,
            (varying vkl_range1f *)&interval.interval,
            &time,
            (void *uniform)intervalIteratorBuffer);
#else
    VKLIntervalIterator intervalIterator =
        vklInitIntervalIteratorV(&vm->vklIntervalContext,
            (varying vkl_vec3f *)&ray.org,
            (varying vkl_vec3f *)&ray.dir,
            (varying vkl_range1f *)&interval.interval,
#ifdef OSPRAY_TARGET_SYCL
            time,
#else
            &time,
#endif
            (void *uniform)intervalIteratorBuffer
#ifdef OSPRAY_TARGET_SYCL
            ,
            ff.volume
#endif
        );
#endif

    VKLInterval interval;
    while (vklIterateIntervalV(intervalIterator,
               &interval
#ifdef OSPRAY_TARGET_SYCL
               ,
               ff.volume
#endif
               )
        && alpha < 0.99f) {
      const float nominalSamplingDt = interval.nominalDeltaT / samplingRate;

      // initial sub interval, based on our renderer-defined sampling rate
      // and the volume's nominal dt
      box1f subInterval = make_box1f(interval.tRange.lower,
          min(interval.tRange.lower + nominalSamplingDt,
              interval.tRange.upper));

      while (subInterval.upper - subInterval.lower > 0.f && alpha < 0.99f) {
        transformedRay.t0 =
            subInterval.lower + (subInterval.upper - subInterval.lower);
        const float dt = subInterval.upper - subInterval.lower;

        // Get volume sample
        vec3f p = transformedRay.org + transformedRay.t0 * transformedRay.dir;
#if OPENVKL_VERSION_MAJOR == 1
        // We know if we have OpenVKL version 1 we're not targetting SYCL
        const float sample = vklComputeSampleV(
            volume->vklSampler, (const varying vkl_vec3f *uniform) & p);
#else
        const float sample = vklComputeSampleV(&volume->vklSampler,
            (const varying vkl_vec3f *uniform) & p
#ifdef OSPRAY_TARGET_SYCL
            ,
            0,
            time,
            ff.volume
#endif
        );
#endif

        if (!isnan(sample)) {
          vec4f sampleColorOpacity =
              TransferFunction_dispatch_get(vm->transferFunction, sample);

          sampleColorOpacity.w = 1.f
              - pow(1.f - sampleColorOpacity.w * vm->densityScale,
                  dt / interval.nominalDeltaT);
          const float clampedOpacity = clamp(sampleColorOpacity.w);
          color = color
              + ((1.f - alpha) * clampedOpacity
                  * make_vec3f(sampleColorOpacity));
          alpha = alpha + ((1.f - alpha) * clampedOpacity);
        }

        // compute next sub interval
        subInterval.lower = subInterval.upper;
        subInterval.upper =
            min(subInterval.lower + nominalSamplingDt, interval.tRange.upper);
      }
    }
  }

  return make_vec4f(color, alpha);
}
#endif

inline float computeAO(const DistributedRaycastRenderer *uniform self,
    const FrameBuffer *uniform fb,
    const World *uniform world,
    const varying vec3i &sampleID,
    const varying DifferentialGeometry &dg,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform int &sampleCnt = self->aoSamples;
  const uniform int accumID = reduce_max(sampleID.z) * sampleCnt;

  // init TEA RNG //
  RandomTEA rng_state;
  varying RandomTEA *const uniform rng = &rng_state;
  RandomTEA_Constructor(rng, 0x290374, (fb->size.x * sampleID.y) + sampleID.x);
  const vec2f rot = RandomTEA_getFloats(rng);

  int hits = 0;

  const linear3f localToWorld = frame(dg.Ns);

  for (uniform int i = 0; i < sampleCnt; i++) {
    const vec2f halton =
        HaltonSequence_get2D(self->super.mathConstants, accumID + i);
    const vec2f r = CranleyPattersonRotation(halton, rot);
    const vec3f local_ao_dir = cosineSampleHemisphere(r);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    if (dot(ao_dir, dg.Ns) < 0.05f) { // check below surface
      hits++;
      continue;
    }

    Ray ao_ray;
    setRay(ao_ray, dg.P, ao_dir, dg.epsilon, self->aoRadius);
    if (isOccluded(world, ao_ray, ffh))
      hits++;
  }

  // the cosTheta of cosineSampleHemispherePDF and dot(shadingNormal, ao_dir)
  // cancel
  return 1.0f - (hits / (float)sampleCnt);
}

inline vec4f DRR_shadeSurface(const DistributedRaycastRenderer *uniform self,
    const FrameBuffer *uniform fb,
    const DistributedWorld *uniform world,
    const vec3i &sampleID,
    const Ray &ray,
    const DifferentialGeometry &dg,
    const uniform FeatureFlagsHandler &ffh)
{
  // TODO: DRR should have its own support for OBJ material and lighting model

  vec3f surfaceColor = make_vec3f(dg.color);
  float opacity = 1.f;
  const OBJ *mat = (const OBJ *)dg.material;
  if (mat) {
#ifdef OSPRAY_TARGET_SYCL
    {
      const OBJ *m = mat;
#else
    foreach_unique (m in mat) {
#endif
      surfaceColor = surfaceColor * m->Kd;
      opacity = m->d;
      if (valid(m->KdMap)) {
        vec4f kdFromMap = get4f(m->KdMap, dg);
        surfaceColor = surfaceColor * make_vec3f(kdFromMap);
      }
    }
  }

  // Just eyelight for now...
  const float eyeLightIntensity = absf(dot(dg.Ns, ray.dir));
  vec3f color = surfaceColor * eyeLightIntensity;
  if (self->aoSamples > 0) {
    float ao = computeAO(self, fb, &world->super, sampleID, dg, ffh);
    color = color * ao;
  }
  return make_vec4f(color, opacity);

#if 0
  // TODO LIGHTS
  // Calculate shading for all lights
  for (uniform int i = 0; self->lights && i < self->numLights; i++) {
    const uniform Light *uniform l = self->lights[i];
    const vec2f s = make_vec2f(0.0f); // sample center of area lights
    const Light_SampleRes light = l->sample(l, dg, s);

    // any potential contribution?
    if (reduce_max(light.weight) > 0.f) {
      float cosNL = dot(light.dir, info.shadingNormal);
      if (cosNL < 0.0f)
        continue;

      cosNL = abs(cosNL);

      const vec3f H = normalize(light.dir - ray.dir);
      const float cosNH = dot(H, info.shadingNormal);
      const vec3f specular = info.Ks * powf(cosNH, info.Ns);
      const vec3f diffuse = info.Kd * cosNL;
      const vec3f light_contrib =
        info.local_opacity * (diffuse + specular) * light.weight;

      if (self->shadowsEnabled) {
        const float max_contrib = reduce_max(light_contrib);
        if (max_contrib > self->super.minContribution) {
          vec3f P = dg.P;
          if (dot(dg.Ng, light.dir) < 0.0f) {
            P = P - (2.f * dg.epsilon) * dg.Ng;
          }

          Ray shadowRay;
          setRay(shadowRay, P, light.dir, 0.0f, light.dist, 0.5f);
          if (!isOccluded(model, shadowRay)
              && !(ghostModel && isOccluded(ghostModel, shadowRay)))
          {
            color = color + light_contrib;
          }
        }
      } else {
        color = color + light_contrib;
      }
    }
  }
#endif
}

// TODO: Better separate geometry and volume code in this function
inline void DRR_renderRegionSample(Renderer *uniform _self,
    SparseFB *uniform fb,
    DistributedWorld *uniform world,
    const box3f *uniform region,
    const vec2f &regionInterval,
    void *uniform perFrameData,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  DistributedRaycastRenderer *uniform self =
      (DistributedRaycastRenderer * uniform) _self;
  const uniform FeatureFlags ff = getFeatureFlags(ffh);

  Ray &geomRay = sample.ray;
  Ray volumeRay = sample.ray;

  traceRay(&world->super, geomRay, ffh);
#ifdef OSPRAY_ENABLE_VOLUMES
  VolumeInterval volumeInterval;
  if (ff.volume) {
    traceVolumeRay(&world->super, volumeRay, volumeInterval);
    volumeInterval.interval.lower =
        max(volumeInterval.interval.lower, regionInterval.x);
    volumeInterval.interval.upper =
        min(volumeInterval.interval.upper, regionInterval.y);

    sample.z = min(geomRay.t, volumeInterval.interval.lower);
  } else
#endif
  {
    sample.z = geomRay.t;
  }

  // TODO: Doesn't seem like much jittering happens with the volume integration
  vec4f outputColor = make_vec4f(0.f);
  while (outputColor.w < 0.99f) {
    const bool haveGeometryHit = ff.geometry ? hadHit(geomRay)
            && geomRay.t >= regionInterval.x && geomRay.t <= regionInterval.y
                                             : false;

#ifdef OSPRAY_ENABLE_VOLUMES
    const bool haveVolumeHit = ff.volume ? hasInterval(volumeInterval) : false;

    const bool bothHit = haveGeometryHit && haveVolumeHit;
    const bool eitherHit = haveGeometryHit || haveVolumeHit;

    const bool volumeFirst =
        ff.volume ? volumeInterval.interval.lower < geomRay.t : false;
#else
    const bool bothHit = false;
    const bool eitherHit = haveGeometryHit;
#endif

    if (!eitherHit) {
      break;
    }

    vec4f volumeColor = make_vec4f(0.f);
    vec4f surfaceColor = make_vec4f(0.f);
    DifferentialGeometry dg;
    if (haveGeometryHit && ff.geometry) {
      postIntersect(&world->super,
          &self->super,
          dg,
          geomRay,
          DG_NG | DG_NS | DG_NORMALIZE | DG_FACEFORWARD | DG_COLOR
              | DG_TEXCOORD,
          ffh);
      surfaceColor = DRR_shadeSurface(
          self, &fb->super, world, sample.sampleID, geomRay, dg, ffh);
    }

#ifdef OSPRAY_ENABLE_VOLUMES
    // Always just integrate the volume when it comes in front of the geometry
    if (haveVolumeHit && volumeFirst && ff.volume) {
      volumeInterval.interval.upper =
          min(geomRay.t, volumeInterval.interval.upper);

      // Keep the interval within the region
      volumeInterval.interval.lower =
          max(volumeInterval.interval.lower, regionInterval.x);
      volumeInterval.interval.upper =
          min(volumeInterval.interval.upper, regionInterval.y);

      // Offset the volume interval start to align to the same sampling steps
      // across each rank, and apply the same jitter for each pixel on all ranks
      float dt = 1.f /*was 'volModel->samplingStep*/ / self->volumeSamplingRate;
      float t0 = volumeInterval.interval.lower;
      volumeInterval.interval.lower = floor(t0 / dt) * dt;

      float jitter = Halton_sample2(sample.sampleID.z);
      int ix = sample.sampleID.x % 4;
      int iy = sample.sampleID.y % 4;

      int patternID = ix + 4 * iy;
      jitter += Halton_sample3(self->super.mathConstants, patternID);
      if (jitter > 1.f) {
        jitter -= 1.f;
      }
      volumeInterval.interval.lower += jitter * dt;

      if (volumeInterval.interval.lower < t0) {
        volumeInterval.interval.lower += dt;
      }

      // TODO: At edges when coloring all bricks the same I do still see
      // some edge/border artifacts

      setRay(volumeRay,
          volumeInterval.interval.lower,
          volumeInterval.interval.upper);
      volumeColor = DRR_integrateVolumeInterval(
          sample, volumeInterval, volumeRay, self->volumeSamplingRate, ffh);
    }
#endif

    vec4f blendedColor = make_vec4f(0.f);
    if (bothHit) {
      blendedColor = volumeColor + (1.0f - volumeColor.w) * surfaceColor;
    } else {
      if (haveGeometryHit)
        blendedColor = surfaceColor;
      else
        blendedColor = volumeColor;
    }

    outputColor = outputColor + (1.0f - outputColor.w) * blendedColor;

    if (haveGeometryHit)
      setRay(geomRay, geomRay.t + dg.epsilon, regionInterval.y + 0.0001f);
    else
      setRay(geomRay, geomRay.t, geomRay.t0);

    // Step the volume ray forwards as well
    volumeRay = geomRay;

    traceRay(&world->super, geomRay, ffh);
#ifdef OSPRAY_ENABLE_VOLUMES
    if (ff.volume) {
      traceVolumeRay(&world->super, volumeRay, volumeInterval);
    }
#endif
  }
  sample.rgb = make_vec3f(outputColor);
  sample.alpha = outputColor.w;
}

// Exported functions /////////////////////////////////////////////////////////

#define renderRegionSampleFn DRR_renderRegionSample
#include "render/distributed/DistributedRendererRenderTaskFn.inl"
#undef renderRegionSampleFn

#ifdef OSPRAY_TARGET_SYCL
SYCL_EXTERNAL void DistributedRaycast_renderRegionToTileTask(
    void *uniform _self,
    void *uniform _fb,
    void *uniform _camera,
    void *uniform _world,
    const void *uniform _region,
    void *uniform perFrameData,
    const void *uniform _taskIDs,
    const int taskIndex0,
    const uniform FeatureFlagsHandler &ffh)
{
  Renderer *uniform self = (Renderer * uniform) _self;
  SparseFB *uniform fb = (SparseFB * uniform) _fb;
  Camera *uniform camera = (Camera * uniform) _camera;
  DistributedWorld *uniform world = (DistributedWorld * uniform) _world;
  const box3f *uniform region = (const box3f *uniform)_region;
  const uint32 *uniform taskIDs = (const uint32 *uniform)_taskIDs;
  DR_default_renderRegionToTile(
      self, fb, camera, world, region, perFrameData, taskIDs, taskIndex0, ffh);
}
#else
export void DistributedRaycast_renderRegionToTileTask(void *uniform _self,
    void *uniform _fb,
    void *uniform _camera,
    void *uniform _world,
    void *uniform _region,
    void *uniform perFrameData,
    void *uniform _taskIDs,
    uniform uint32 numTasks)
{
  Renderer *uniform self = (Renderer * uniform) _self;
  SparseFB *uniform fb = (SparseFB * uniform) _fb;
  Camera *uniform camera = (Camera * uniform) _camera;
  DistributedWorld *uniform world = (DistributedWorld * uniform) _world;
  const box3f *uniform region = (const box3f *uniform)_region;
  const uint32 *uniform taskIDs = (const uint32 *uniform)_taskIDs;
  uniform FeatureFlagsHandler ffh;
  launch[numTasks] DR_default_renderRegionToTile(
      self, fb, camera, world, region, perFrameData, taskIDs, ffh);
  sync;
}
#endif

OSPRAY_END_ISPC_NAMESPACE
