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
    VolumetricModel *uniform m,
    const uniform float samplingRate)
{
  // We have to iterate till we get a valid sample value
  float newDistance;
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
            (vc.distance == inf) ? vc.interval.tRange.lower : vc.distance;

      } else {
        // The end of the volume has been reached
        vc.distance = inf;
        return;
      }
    }

    // Calculate sampling distance
    const float samplingStep = vc.interval.nominalDeltaT / samplingRate;
    newDistance = vc.interval.tRange.lower + vc.iuDistance * samplingStep;

    // Prepare sampling position
    const vec3f p = vc.org + newDistance * vc.dir;

    // Sample volume value in given point
    sampleVal = vklComputeSampleV(
        m->volume->vklVolume, (const varying vkl_vec3f *uniform) & p);

    // Go to the next sub-interval
    vc.iuDistance += 1.f;
  }

  // Apply transfer function to get color with alpha
  vc.sample = m->transferFunction->get(m->transferFunction, sampleVal);

  // Weight the opacity with deltaT using Beer law
  const float dt = newDistance - vc.distance;
  vc.sample.w = 1.f - exp(-vc.sample.w * dt * m->densityScale);
  vc.distance = newDistance;
}

static float sampleAllVolumes(const VolumeIntervals &volumeIntervals,
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
        sampleVolume(vc, m, samplingRate);
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
    Ray &ray,
    varying LDSampler *uniform ldSampler,
    const uniform float samplingRate)
{
  // Array of volume contexts
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
    vc.ready = 0;

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
  }

  // Define initial color with alpha
  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

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

  // Return final color
  return make_vec4f(color, alpha);
}
