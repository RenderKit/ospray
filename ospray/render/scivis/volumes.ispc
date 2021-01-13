// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "surfaces.ih"
#include "volumes.ih"
// ispc device
#include "math/random.ih"
#include "math/sampling.ih"
#include "render/util.ih"

#include "openvkl/openvkl.isph"

struct VolumeContext
{
  uniform unsigned int8 intervalIteratorBuffer[VKL_MAX_INTERVAL_ITERATOR_SIZE];
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

static void sampleVolume(SciVisRenderContext &rc,
    VolumeContext &vc,
    VolumetricModel *uniform m,
    Ray &ray,
    const VolumeInterval &vi,
    const uniform float samplingRate,
    const uniform bool shade)
{
  // We have to iterate till we get a valid sample value
  float dt;
  float sampleVal = nan;
  const uniform float gsc = shade ? m->gradientShadingScale : 0.f;
  vec3f p; // in volume local coords
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
    const float newDistance =
        vc.interval.tRange.lower + vc.iuDistance * samplingStep;

    // Prepare sampling position
    p = vc.org + newDistance * vc.dir;

    // Sample volume value in given point
    sampleVal = vklComputeSampleV(
        m->volume->vklSampler, (const varying vkl_vec3f *uniform) & p);

    // Go to the next sub-interval
    vc.iuDistance += 1.f;
    dt = newDistance - vc.distance - emptySpace;
    vc.distance = newDistance;
  }

  // Apply transfer function to get color with alpha
  vc.sample = m->transferFunction->get(m->transferFunction, sampleVal);

  // compute gradient shading lighting
  if (gsc > 0.0f) {
    vec3f ns = Volume_getGradient(m->volume, p);
    if (dot(ns, ns) > 1e-6f) {
      // assume that opacity directly correlates to volume scalar field, i.e.
      // that "outside" has lower values; because the gradient point towards
      // increasing values we need to flip it
      ns = neg(ns);

      // use scivis shading function
      // Prepare differential geometry structure
      DifferentialGeometry dg;
      dg.color = vc.sample;
      dg.material = NULL;
      dg.epsilon = dt / 2.f;
      // transform to world coords
      dg.Ns = dg.Ng = normalize(xfmVector(transposed(vi.xfm.l), ns));
      dg.P = ray.org + vc.distance * ray.dir;
      SSI shading = computeShading(rc.renderer,
          rc.fb,
          rc.world,
          dg,
          rc.sample,
          rc.ldSampler,
          ray.dir,
          0.f);
      vc.sample = lerp(gsc, vc.sample, shading.shadedColor);
    }
  }

  // Weight the opacity with deltaT using Beer law
  vc.sample.w = 1.f - exp(-vc.sample.w * dt * m->densityScale);
}

static float sampleAllVolumes(SciVisRenderContext &rc,
    const VolumeIntervals &volumeIntervals,
    varying VolumeContext *uniform volumeContexts,
    Ray &ray,
    const uniform float samplingRate,
    vec4f &sampledColor,
    const uniform bool shade)
{
  // Look for the closest sample across all volumes
  float minDist = inf;
  int usedSampleId = -1;
  for (uniform int i = 0; i < reduce_max(volumeIntervals.numVolumeIntervals);
       i++) {
    if (i >= volumeIntervals.numVolumeIntervals)
      break;

    // If the sample has been used already regenerate a new one
    VolumeContext &vc = volumeContexts[i];
    if (vc.ready == 0) {
      const VolumeInterval &vi = volumeIntervals.intervals[i];
      foreach_unique (m in vi.volumetricModel) {
        sampleVolume(rc, vc, m, ray, vi, samplingRate, shade);
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

vec4f integrateVolumeIntervalsGradient(SciVisRenderContext &rc,
    const VolumeIntervals &volumeIntervals,
    const RayIntervals &rayIntervals,
    Ray &ray,
    varying LDSampler *uniform ldSampler,
    const uniform float samplingRate,
    const uniform bool shade)
{
  // Array of volume contexts
  VolumeContext volumeContexts[NUM_VOLUME_INTERVALS_MAX];

  // Sampling position jitter
  const float jitter = LDSampler_getFloat(ldSampler, 0);

  // Iterate through all volumes and initialize its contexts with data that
  // do not change across ray intervals
  for (uniform int i = 0; i < reduce_max(volumeIntervals.numVolumeIntervals);
       i++) {
    if (i >= volumeIntervals.numVolumeIntervals)
      break;

    // Transform ray into the volume local space
    const VolumeInterval &vi = volumeIntervals.intervals[i];
    Ray transformedRay = ray;
    transformRay(transformedRay, vi.rcp_xfm);

    // Set volume context initial values
    VolumeContext &vc = volumeContexts[i];
    vc.org = transformedRay.org;
    vc.dir = transformedRay.dir;
  }

  // Define initial color with alpha
  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  // Iterate through all ray intervals
  for (uniform int i = 0; i < reduce_max(rayIntervals.count) && (alpha < .99f);
       i++) {
    if (i >= rayIntervals.count)
      break;

    // Iterate through all volumes
    for (uniform int j = 0; j < reduce_max(volumeIntervals.numVolumeIntervals);
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
      VolumetricModel *varying model = vi.volumetricModel;
      foreach_unique (m in model) {
        // Create volume interval interator
        vc.intervalIterator = vklInitIntervalIteratorV(m->volume->vklSampler,
            (varying vkl_vec3f *)&vc.org,
            (varying vkl_vec3f *)&vc.dir,
            (varying vkl_range1f *)&rInterval,
            m->vklValueSelector,
            vc.intervalIteratorBuffer);
      }
    }

    // Propagate ray across all volumes till opaque
    while (alpha < .99f) {
      // Sample across all volumes
      vec4f sampledColorOpacity;
      float dist = sampleAllVolumes(rc,
          volumeIntervals,
          volumeContexts,
          ray,
          samplingRate,
          sampledColorOpacity,
          shade);

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

  // Return final color
  return make_vec4f(color, alpha);
}
