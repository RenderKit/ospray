// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#include "math/random.ih"
#include "math/sampling.ih"
#include "render/util.ih"
#include "volume/Volume.ih"
#include "volume/VolumetricModel.ih"
#include "volume/transferFunction/TransferFunctionDispatch.ih"
#include "volumes.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

#ifndef OSPRAY_TARGET_SYCL

struct VolumeContext
{
  uniform uint8 intervalIteratorBuffer[VKL_MAX_INTERVAL_ITERATOR_SIZE];
  VKLIntervalIterator intervalIterator;
  VKLInterval interval;
  vec3f org;
  vec3f dir;
  float iuLength; // VKL interval length in sampling steps as an unit
  float iuDistance; // sampling distance within VKL interval in sampling steps
                    // as an unit
  float distance; // last sampling distance from 'vc.org'
  vec4f sample;
  uint32 ready; // 1 if sample is ready to be used
};

static void sampleVolume(VolumeContext &vc,
    const VolumetricModel *uniform m,
    const uniform float samplingRate)
{
  // We have to iterate till we get a valid sample value
  float dt = 0.f;
  float sampleVal = nan;
  while (isnan(sampleVal)) {
    // Iterate till sampling position is within interval
    float emptySpace = 0.f;
    while (vc.iuDistance > vc.iuLength) {
      // Get next VKL interval
      const float prevUpper = vc.interval.tRange.upper;
      if (vklIterateIntervalV(vc.intervalIterator, &vc.interval)) {
        // Intervals may not be contiguous, accumulate empty space
        emptySpace += max(vc.interval.tRange.lower - prevUpper, 0.f);

        // Make it local for the next interval
        vc.iuDistance -= vc.iuLength;

        // Calculate how many steps can be made within this interval
        const float samplingStep = vc.interval.nominalDeltaT / samplingRate;
        vc.iuLength = (vc.interval.tRange.upper - vc.interval.tRange.lower)
            / samplingStep;

        // Initialize distance if necessary
        vc.distance =
            (vc.distance == inf) ? vc.interval.tRange.lower : vc.distance;

      } else {
        // The end of the volume has been reached
        vc.distance = inf;
        return;
      }
    }

    // Calculate sampling distance
    const float samplingStep = vc.interval.nominalDeltaT / samplingRate;
    const float newDistance =
        vc.interval.tRange.lower + vc.iuDistance * samplingStep;

    // Prepare sampling position
    const vec3f p = vc.org + newDistance * vc.dir;

    // Sample volume value in given point
    sampleVal = vklComputeSampleV(
        &m->volume->vklSampler, (const varying vkl_vec3f *uniform) & p);
    // Go to the next sub-interval
    vc.iuDistance += 1.f;
    dt = newDistance - vc.distance - emptySpace;
    vc.distance = newDistance;
  }

  // Apply transfer function to get color with alpha
  vc.sample = TransferFunction_dispatch_get(m->transferFunction, sampleVal);

  // Weight the opacity with deltaT using Beer law
  vc.sample.w = 1.f - exp(-vc.sample.w * dt * m->densityScale);
}

static float sampleAllVolumes(const VolumeIntervals &volumeIntervals,
    varying VolumeContext *uniform volumeContexts,
    const uniform float samplingRate,
    vec4f &sampledColor)
{
  // Look for the closest sample across all volumes
  float minDist = inf;
  int usedSampleId = -1;
  for (uniform uint32 i = 0; i < reduce_max(volumeIntervals.numVolumeIntervals);
       i++) {
    if (i >= volumeIntervals.numVolumeIntervals)
      break;

    // If the sample has been used already regenerate a new one
    VolumeContext &vc = volumeContexts[i];
    if (vc.ready == 0) {
      const VolumeInterval &vi = volumeIntervals.intervals[i];
      foreach_unique (m in vi.volumetricModel)
        sampleVolume(vc, m, samplingRate);
      vc.ready = 1;
    }

    // Take the sample if closer
    if (vc.distance < minDist) {
      minDist = vc.distance;
      sampledColor = vc.sample;
      usedSampleId = i;
    }
  }

  // Mark used samples as not ready
  if (usedSampleId != -1) {
    volumeContexts[usedSampleId].ready = 0;
  }

  // Return distance for sampled color
  return minDist;
}

SYCL_EXTERNAL vec4f integrateVolumeIntervals(
    const VolumeIntervals &volumeIntervals,
    const RayIntervals &rayIntervals,
    Ray &ray,
    RandomSampler &randomSampler,
    const uniform float samplingRate,
    const uniform FeatureFlagsHandler &)
{
  // Array of volume contexts
  varying VolumeContext *uniform volumeContexts =
      (varying VolumeContext * uniform)
          pushTLS(reduce_max(volumeIntervals.numVolumeIntervals)
              * sizeof(varying VolumeContext));

  // Sampling position jitter
  const float jitter = RandomSampler_getFloat(&randomSampler);

  // Iterate through all volumes and initialize its contexts with data that
  // do not change across ray intervals
  for (uniform uint32 i = 0; i < reduce_max(volumeIntervals.numVolumeIntervals);
       i++) {
    if (i >= volumeIntervals.numVolumeIntervals)
      break;

    // Transform ray into the volume local space
    const VolumeInterval &vi = volumeIntervals.intervals[i];
    Ray transformedRay = ray;
    AffineSpace3f instanceRcpXfm = vi.instance->rcp_xfm;
    transformRay(transformedRay, instanceRcpXfm);

    // Set volume context initial values
    VolumeContext &vc = volumeContexts[i];
    vc.org = transformedRay.org;
    vc.dir = transformedRay.dir;
  }

  // Define initial color with alpha
  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  // Iterate through all ray intervals
  for (uniform uint32 i = 0;
       i < reduce_max(rayIntervals.count) && (alpha < .99f);
       i++) {
    if (i >= rayIntervals.count)
      break;

    // Iterate through all volumes
    for (uniform uint32 j = 0;
         j < reduce_max(volumeIntervals.numVolumeIntervals);
         j++) {
      if (j >= volumeIntervals.numVolumeIntervals)
        break;

      // Calculate volume and ray intervals union
      const VolumeInterval &vi = volumeIntervals.intervals[j];
      range1f rInterval = rayIntervals.intervals[i];
      rInterval.lower = max(rInterval.lower, vi.interval.lower);
      rInterval.upper = min(rInterval.upper, vi.interval.upper);

      // Reset distance to sample
      VolumeContext &vc = volumeContexts[j];
      vc.distance = inf;

      // Check if volume and ray intervals overlap
      if (rInterval.upper <= rInterval.lower) {
        // Skip this volume if not
        vc.ready = 1;
        continue;
      }

      // We will sample across this volume so initialize members used during
      // sampling
      vc.iuDistance = jitter;
      vc.iuLength = 0.f;
      vc.ready = 0;
      vc.interval.tRange.upper = inf;

      // There might be different volumetric models used across vector lanes
      // so we must iterate over them
      float time = 0.5f;
      VolumetricModel *varying model = vi.volumetricModel;
      // Create volume interval iterator
      foreach_unique (m in model)
        vc.intervalIterator = vklInitIntervalIteratorV(&m->vklIntervalContext,
            (varying vkl_vec3f *)&vc.org,
            (varying vkl_vec3f *)&vc.dir,
            (varying vkl_range1f *)&rInterval,
            &time,
            vc.intervalIteratorBuffer);
    }

    // Propagate ray across all volumes till opaque
    while (alpha < .99f) {
      // Sample across all volumes
      vec4f sampledColorOpacity;
      float dist = sampleAllVolumes(
          volumeIntervals, volumeContexts, samplingRate, sampledColorOpacity);

      // Exit loop if nothing sampled
      if (dist == inf)
        break;

      // Alpha blend sampled color
      color = color
          + ((1.f - alpha) * sampledColorOpacity.w
              * make_vec3f(sampledColorOpacity));
      alpha = alpha + ((1.f - alpha) * sampledColorOpacity.w);
    }
  }

  popTLS(volumeContexts);

  // Return final color
  return make_vec4f(color, alpha);
}

#else

// GPU version
SYCL_EXTERNAL vec4f integrateVolumeIntervals(
    const VolumeIntervals &volumeIntervals,
    const RayIntervals &,
    Ray &ray,
    RandomSampler &randomSampler,
    const uniform float samplingRate,
    const uniform FeatureFlagsHandler &ffh)
{
  const VolumeInterval &vi = volumeIntervals.intervals[0];
  Ray tRay = ray;
  AffineSpace3f instanceRcpXfm = vi.instance->rcp_xfm;
  transformRay(tRay, instanceRcpXfm);

  // Define initial color with alpha
  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  const uniform FeatureFlags ff = getFeatureFlags(ffh);

  uniform uint8 intervalIteratorBuffer[VKL_MAX_INTERVAL_ITERATOR_SIZE];
  VolumetricModel *varying m = vi.volumetricModel;
  VKLIntervalIterator intervalIterator =
      vklInitIntervalIteratorV(&m->vklIntervalContext,
          (varying vkl_vec3f *)&tRay.org,
          (varying vkl_vec3f *)&tRay.dir,
          (varying vkl_range1f *)&vi.interval,
          .5f,
          intervalIteratorBuffer,
          ff.volume);

  // Sampling position jitter
  const float jitter = RandomSampler_getFloat(&randomSampler);

  // Propagate ray across all volumes till opaque
  float distance = inf;
  float iuDistance = jitter;
  float iuLength = 0.f;
  VKLInterval interval;
  interval.tRange.upper = inf;
  float samplingStep = 0.f;
  while (alpha < .99f) {
    // Iterate till sampling position is within interval
    float emptySpace = 0.f;
    while (iuDistance > iuLength) {
      // Get next VKL interval
      const float prevUpper = interval.tRange.upper;
      if (vklIterateIntervalV(intervalIterator, &interval, ff.volume)) {
        // Intervals may not be contiguous, accumulate empty space
        emptySpace += max(interval.tRange.lower - prevUpper, 0.f);

        // Make it local for the next interval
        iuDistance -= iuLength;

        // Calculate how many steps can be made within this interval
        samplingStep = interval.nominalDeltaT / samplingRate;
        iuLength =
            (interval.tRange.upper - interval.tRange.lower) / samplingStep;

        // Initialize distance if necessary
        distance = (distance == inf) ? interval.tRange.lower : distance;

      } else {
        // The end of the volume has been reached
        return make_vec4f(color, alpha);
      }
    }

    // Calculate sampling distance
    const float newDistance = interval.tRange.lower + iuDistance * samplingStep;

    // Prepare sampling position
    const vec3f p = tRay.org + newDistance * tRay.dir;

    // Sample volume value in given point
    float sampleVal = vklComputeSampleV(&m->volume->vklSampler,
        (const varying vkl_vec3f *uniform) & p,
        0,
        .5f,
        ff.volume);

    if (!isnan(sampleVal)) {
      // Apply transfer function to get color with alpha
      vec4f sampledColor =
          TransferFunction_dispatch_get(m->transferFunction, sampleVal);

      // Weight the opacity with deltaT using Beer law
      float dt = newDistance - distance - emptySpace;
      sampledColor.w = 1.f - exp(-sampledColor.w * dt * m->densityScale);

      // Alpha blend sampled color
      color =
          color + ((1.f - alpha) * sampledColor.w * make_vec3f(sampledColor));
      alpha = alpha + ((1.f - alpha) * sampledColor.w);
    }

    // Go to the next sub-interval
    iuDistance += 1.f;
    distance = newDistance;
  }

  // Return final color
  return make_vec4f(color, alpha);
}

#endif

OSPRAY_END_ISPC_NAMESPACE
#endif
