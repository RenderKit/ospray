// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "volumes.ih"
// ispc device
#include "math/random.ih"
#include "math/sampling.ih"
#include "render/util.ih"

#include "openvkl/openvkl.isph"

struct VolumeContext
{
  VKLIntervalIterator intervalIterator;
  VKLInterval interval;
  uint32 rayIntervalId;
  vec3f org;
  vec3f dir;
  float iuLength; // VKL interval length in sampling steps as an unit
  float iuDistance; // sampling distance within VKL interval in sampling steps
                    // as an unit
  float distance; // last sampling distance from 'vc.org'
  vec4f sample;
  uint32 ready; // 1 if sample is ready to be used
};

static bool inInterval(const RayIntervals &rayIntervals, VolumeContext &vc)
{
  do {
    // Value before the current interval
    if (rayIntervals.intervals[vc.rayIntervalId].lower > vc.distance)
      return false;

    // Value in the current interval
    if (rayIntervals.intervals[vc.rayIntervalId].upper > vc.distance)
      return true;

    // Go to the next interval
    vc.rayIntervalId++;

  } while (vc.rayIntervalId < rayIntervals.count);

  // No more intervals to test
  vc.distance = inf;
  return false;
}

static void sampleVolume(const RayIntervals &rayIntervals,
    VolumeContext &vc,
    VolumetricModel *uniform m,
    const uniform float samplingRate)
{
  // We have to iterate till we get a valid sample value
  float prevDistance;
  float sampleVal = nan;
  while (isnan(sampleVal)) {
    // Iterate till sampling position is within interval
    while (vc.iuDistance > vc.iuLength) {
      // Get next VKL interval
      if (vklIterateIntervalV(&vc.intervalIterator, &vc.interval)) {
        // Make it local for the next interval
        vc.iuDistance -= vc.iuLength;

        // Calulate how many steps can be made within this interval
        const float samplingStep = vc.interval.nominalDeltaT / samplingRate;
        vc.iuLength = (vc.interval.tRange.upper - vc.interval.tRange.lower)
            / samplingStep;

        // Initialize distance if necessary
        vc.distance =
            select(vc.distance == inf, vc.interval.tRange.lower, vc.distance);

      } else {
        // The end of the volume has been reached
        vc.distance = inf;
        return;
      }
    }

    // Calculate sampling distance
    const float samplingStep = vc.interval.nominalDeltaT / samplingRate;
    prevDistance = vc.distance;
    vc.distance = vc.interval.tRange.lower + vc.iuDistance * samplingStep;

    // Check if sampling position is not clipped out
    if (inInterval(rayIntervals, vc)) {
      // Prepare sampling position
      const vec3f p = vc.org + vc.distance * vc.dir;

      // Sample volume value in given point
      sampleVal = vklComputeSampleV(
          m->volume->vklVolume, (const varying vkl_vec3f *uniform) & p);
    } else if (vc.distance == inf) {
      return;
    }

    // Go to the next sub-interval
    vc.iuDistance += 1.f;
  }

  // Apply transfer function to get color with alpha
  vc.sample = m->transferFunction->get(m->transferFunction, sampleVal);

  // Weight the opacity with deltaT using Beer law
  const float dt = vc.distance - prevDistance;
  vc.sample.w = 1.f - exp(-vc.sample.w * dt * m->densityScale);
}

static float sampleAllVolumes(const VolumeIntervals &volumeIntervals,
    const RayIntervals &rayIntervals,
    varying VolumeContext *uniform volumeContexts,
    const uniform float samplingRate,
    vec4f &sampledColor)
{
  // Look for the closest sample across all volumes
  float minDist = inf;
  int usedSampleId = -1;
  for (uniform int i = 0; i < reduce_max(volumeIntervals.numVolumeIntervals);
       i++) {
    // Uniform loop counter is used so we have to mask individual lanes manually
    if (i >= volumeIntervals.numVolumeIntervals)
      break;

    // Get volume data
    const varying VolumeInterval *uniform vi = volumeIntervals.intervals + i;
    VolumeContext &vc = volumeContexts[i];

    // If the sample has been used already regenerate a new one
    if (vc.ready == 0) {
      foreach_unique (m in vi->volumetricModel) {
        sampleVolume(rayIntervals, vc, m, samplingRate);
      }
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

vec4f integrateVolumeIntervals(const VolumeIntervals &volumeIntervals,
    const RayIntervals &rayIntervals,
    Ray &ray,
    varying LDSampler *uniform ldSampler,
    const uniform float samplingRate)
{
  // Keep context for each volume here
  VolumeContext volumeContexts[NUM_VOLUME_INTERVALS_MAX];

  // Sampling position jitter
  const float jitter = LDSampler_getFloat(ldSampler, 0);

  // Iterate through all volumes and initialize its contexts
  for (uniform int i = 0; i < reduce_max(volumeIntervals.numVolumeIntervals);
       i++) {
    // Uniform loop counter is used so we have to mask individual lanes manually
    if (i >= volumeIntervals.numVolumeIntervals)
      break;

    // Transform ray into the volume local space
    const varying VolumeInterval *uniform vi = volumeIntervals.intervals + i;
    Ray transformedRay = ray;
    transformRay(transformedRay, vi->rcp_xfm);

    // Set volume context initial values
    VolumeContext &vc = volumeContexts[i];
    vc.org = transformedRay.org;
    vc.dir = transformedRay.dir;
    vc.iuDistance = jitter;
    vc.iuLength = 0.f;
    vc.distance = inf;

    // Find initial ray interval indices
    int j = 0;
    for (; j < rayIntervals.count; j++) {
      const range1f rayInterval = rayIntervals.intervals[j];
      if (overlapped(rayInterval, vi->interval)) {
        vc.rayIntervalId = j;
        break;
      }
    }

    // If no overlapping interval found
    if (j == rayIntervals.count) {
      // No need to sample
      vc.ready = 1;

    } else {
      // There might be different volumetric models used across vector lanes
      // so we must iterate over them
      VolumetricModel *varying model = vi->volumetricModel;
      foreach_unique (m in model) {
        // Create volume interval interator
        vklInitIntervalIteratorV(&vc.intervalIterator,
            m->volume->vklVolume,
            (varying vkl_vec3f *)&transformedRay.org,
            (varying vkl_vec3f *)&transformedRay.dir,
            (varying vkl_range1f *)&vi->interval,
            m->vklValueSelector);
      }

      // Need to sample
      vc.ready = 0;
    }
  }

  // Define initial color with alpha
  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  // Propagate ray across all volumes till opaque
  while (alpha < .99f) {
    // Sample across all volumes
    vec4f sampledColorOpacity;
    float dist = sampleAllVolumes(volumeIntervals,
        rayIntervals,
        volumeContexts,
        samplingRate,
        sampledColorOpacity);

    // Exit loop if nothing sampled
    if (dist == inf)
      break;

    // Alpha blend sampled color
    color = color
        + ((1.f - alpha) * sampledColorOpacity.w
            * make_vec3f(sampledColorOpacity));
    alpha = alpha + ((1.f - alpha) * sampledColorOpacity.w);
  }

  // Return final color
  return make_vec4f(color, alpha);
}
