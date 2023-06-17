// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#include "common/FeatureFlagsEnum.h"
#include "common/World.ih"
#include "render/pathtracer/volumes/VolumeSampler.ih"
#include "volume/transferFunction/TransferFunctionDispatch.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

float delta_tracking(const VolumetricModel *uniform vModel,
    const range1f &rInterval,
    varying RandomSampler *uniform randomSampler,
    const vec3f &o,
    const vec3f &w,
    const float &time,
    float &mu_t, // sampled extinction coefficint
    vec3f &albedo,
    const uniform FeatureFlagsHandler &ffh) // sampled albedo (color)
{
  const uniform FeatureFlags ff = getFeatureFlags(ffh);

#ifdef OSPRAY_TARGET_SYCL
  // No alloca on GPU, need to be conservative for now and use the
  // VKL_MAX_INTERVAL_ITERATOR_SIZE
  uniform uint8 intervalIteratorBuffer[VKL_MAX_INTERVAL_ITERATOR_SIZE];
#else
  void *uniform intervalIteratorBuffer = alloca(vklGetIntervalIteratorSizeV(
#if OPENVKL_VERSION_MAJOR == 1
      vModel->vklIntervalContext
#else
      &vModel->vklIntervalContext
#endif
      ));
#endif

#if OPENVKL_VERSION_MAJOR == 1
  // We know if we have OpenVKL version 1 we're not targetting SYCL
  VKLIntervalIterator intervalIterator =
      vklInitIntervalIteratorV(vModel->vklIntervalContext,
          (varying vkl_vec3f *)&o,
          (varying vkl_vec3f *)&w,
          (varying vkl_range1f *)&rInterval,
          &time,
          intervalIteratorBuffer);
#else
  VKLIntervalIterator intervalIterator =
      vklInitIntervalIteratorV(&vModel->vklIntervalContext,
          (varying vkl_vec3f *)&o,
          (varying vkl_vec3f *)&w,
          (varying vkl_range1f *)&rInterval,
#ifdef OSPRAY_TARGET_SYCL
          time,
#else
          &time,
#endif
          intervalIteratorBuffer
#ifdef OSPRAY_TARGET_SYCL
          ,
          ff.volume
#endif
      );
#endif
  float t = 0.f;
  VKLInterval interval;
  while (vklIterateIntervalV(intervalIterator,
      &interval
#ifdef OSPRAY_TARGET_SYCL
      ,
      ff.volume
#endif
      )) {
    t = interval.tRange.lower;

    const float maxOpacity =
        TransferFunction_dispatch_getMaxOpacity(vModel->transferFunction,
            *((varying range1f * uniform) & interval.valueRange));

    const float mu_max = vModel->densityScale * maxOpacity;
    if (mu_max <= 0.f)
      continue;

    while (true) {
      float xi = RandomSampler_getFloat(randomSampler);
      const float dt = -log(1.f - xi) / mu_max;
      t += dt;
      if (t > interval.tRange.upper)
        break;

      xi = RandomSampler_getFloat(randomSampler);
      const vec3f p = o + t * w;

#if OPENVKL_VERSION_MAJOR == 1
      // We know if we have OpenVKL version 1 we're not targetting SYCL
      const float sample = vklComputeSampleV(
          vModel->volume->vklSampler, (const varying vkl_vec3f *uniform) & p);
#else
      const float sample = vklComputeSampleV(&vModel->volume->vklSampler,
          (const varying vkl_vec3f *uniform) & p
#ifdef OSPRAY_TARGET_SYCL
          ,
          0,
          time,
          ff.volume
#endif
      );
#endif
      if (isnan(sample))
        continue;

      const vec4f color =
          TransferFunction_dispatch_get(vModel->transferFunction, sample);
      mu_t = vModel->densityScale * color.w;
      if (xi < mu_t / mu_max) {
        albedo = make_vec3f(color);
        return t;
      }
    }
  }
  return inf;
}

SYCL_EXTERNAL float volumeSampleFreePath(const World *uniform world,
    Ray &ray,
    RayIntervals &rayIntervals,
    varying RandomSampler *uniform randomSampler,
    const VolumetricModel *varying *uniform sampledInstance,
    float &sampledExtinctionCoefficient,
    vec3f &sampledAlbedo,
    const uniform FeatureFlagsHandler &ffh)
{
#ifdef OSPRAY_TARGET_SYCL
  // We only support a single volume interval on the GPU
  VolumeInterval vInterval;
  VolumeIntervals volumeIntervals;
  volumeIntervals.numVolumeIntervals = 0;
  volumeIntervals.numAllocated = 1;
  volumeIntervals.intervals = &vInterval;
#else
  VolumeIntervals volumeIntervals;
  allocVolumeIntervals(volumeIntervals);
#endif

  // Get volume intervals
  Ray volumeRay = ray;
  traceVolumeRay(world, volumeRay, volumeIntervals);

  // Iterate through volume intervals
  float min_dist = inf;
  unmasked
  {
    *sampledInstance = NULL;
  }
  for (uniform uint32 i = 0; i < reduce_max(volumeIntervals.numVolumeIntervals);
       i++) {
    if (i >= volumeIntervals.numVolumeIntervals)
      break;

    range1f vInterval = volumeIntervals.intervals[i].interval;
    vInterval.lower = max(0.f, vInterval.lower);
    vInterval.upper =
        noHit(ray) ? vInterval.upper : min(vInterval.upper, ray.t);

    if (isEmpty(vInterval))
      continue;

    AffineSpace3f xfm;
#ifdef OSPRAY_TARGET_SYCL
    {
      const Instance *inst = volumeIntervals.intervals[i].instance;
#else
    foreach_unique (inst in volumeIntervals.intervals[i].instance) {
#endif
      xfm = Instance_getTransform(inst, volumeRay.time);
    }
    Ray transformedVolumeRay = volumeRay;
    transformRay(transformedVolumeRay, rcp(xfm));

    VolumetricModel *varying instance =
        volumeIntervals.intervals[i].volumetricModel;
#ifdef OSPRAY_TARGET_SYCL
    {
      const VolumetricModel *inst = instance;
#else
    foreach_unique (inst in instance) {
#endif
      for (uint32 j = 0; j < rayIntervals.count; j++) {
        range1f rInterval = rayIntervals.intervals[j];

        // Skip ray intervals outside of the volume range
        if (vInterval.upper < rInterval.lower
            || rInterval.upper < vInterval.lower) {
          continue;
        }

        // Clip ray interval to the volume range
        rInterval.lower = max(rInterval.lower, vInterval.lower);
        rInterval.upper = min(rInterval.upper, vInterval.upper);

        float extinctionCoefficient;
        vec3f albedo;
        const float dist = delta_tracking(inst,
            rInterval,
            randomSampler,
            transformedVolumeRay.org,
            transformedVolumeRay.dir,
            transformedVolumeRay.time,
            extinctionCoefficient,
            albedo,
            ffh);
        if (dist < inf) {
          if (dist < min_dist) {
            min_dist = dist;
            *sampledInstance = inst;
            sampledExtinctionCoefficient = extinctionCoefficient;
            sampledAlbedo = albedo;
            ray.t = dist;
            ray.primID = 0;
            ray.geomID = volumeIntervals.intervals[i].geomID;
            ray.instID = volumeIntervals.intervals[i].instID;
          }
          break;
        }
      }
    }
  }
#ifndef OSPRAY_TARGET_SYCL
  freeVolumeIntervals(volumeIntervals);
#endif
  return min_dist;
}

SYCL_EXTERNAL float volumeTransmittance(const World *uniform world,
    Ray &ray,
    RayIntervals &rayIntervals,
    varying RandomSampler *uniform randomSampler,
    const uniform FeatureFlagsHandler &ffh)
{
#ifdef OSPRAY_TARGET_SYCL
  // We only support a single volume interval on the GPU
  VolumeInterval vInterval;
  VolumeIntervals volumeIntervals;
  volumeIntervals.numVolumeIntervals = 0;
  volumeIntervals.numAllocated = 1;
  volumeIntervals.intervals = &vInterval;
#else
  VolumeIntervals volumeIntervals;
  allocVolumeIntervals(volumeIntervals);
#endif

  // Get volume intervals
  Ray volumeRay = ray;
  traceVolumeRay(world, volumeRay, volumeIntervals);

  // Iterate through volume intervals
  float transmittance = 1.f;
  for (uint32 i = 0;
       i < volumeIntervals.numVolumeIntervals && transmittance > 0.f;
       i++) {
    range1f vInterval = volumeIntervals.intervals[i].interval;
    vInterval.lower = max(ray.t0, vInterval.lower);
    vInterval.upper =
        noHit(ray) ? vInterval.upper : min(vInterval.upper, ray.t);

    if (isEmpty(vInterval))
      continue;

    AffineSpace3f xfm;
#ifdef OSPRAY_TARGET_SYCL
    {
      const Instance *inst = volumeIntervals.intervals[i].instance;
#else
    foreach_unique (inst in volumeIntervals.intervals[i].instance) {
#endif
      xfm = Instance_getTransform(inst, volumeRay.time);
    }
    Ray transformedVolumeRay = volumeRay;
    transformRay(transformedVolumeRay, rcp(xfm));

    VolumetricModel *varying instance =
        volumeIntervals.intervals[i].volumetricModel;
#ifdef OSPRAY_TARGET_SYCL
    {
      const VolumetricModel *inst = instance;
#else
    foreach_unique (inst in instance) {
#endif
      for (uint32 j = 0; j < rayIntervals.count && transmittance > 0.f; j++) {
        range1f rInterval = rayIntervals.intervals[j];

        // Skip ray intervals outside of the volume range
        if (vInterval.upper < rInterval.lower
            || rInterval.upper < vInterval.lower) {
          continue;
        }

        // Clip ray interval to the volume range
        rInterval.lower = max(rInterval.lower, vInterval.lower);
        rInterval.upper = min(rInterval.upper, vInterval.upper);

        float extinctionCoefficient;
        vec3f albedo;
        const float dist = delta_tracking(inst,
            rInterval,
            randomSampler,
            transformedVolumeRay.org,
            transformedVolumeRay.dir,
            transformedVolumeRay.time,
            extinctionCoefficient,
            albedo,
            ffh);
        transmittance = transmittance * ((dist < rInterval.upper) ? 0.f : 1.f);
      }
    }
  }
#ifndef OSPRAY_TARGET_SYCL
  freeVolumeIntervals(volumeIntervals);
#endif
  return transmittance;
}

OSPRAY_END_ISPC_NAMESPACE
#endif
