// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "volumes.ih"
// ispc device
#include "math/random.ih"
#include "math/sampling.ih"
#include "render/util.ih"

#include "openvkl/openvkl.isph"

struct VolumeContext
{
  VKLIntervalIterator *intervalIterator;
  VKLInterval interval;
  vec3f org;
  vec3f dir;
  float jitter;
  range1f subInterval;
  float distance;
  vec4f sample;
  uint32 ready;
};

static void sampleVolume(VolumeContext &vc,
                         uniform VKLIntervalIterator *uniform ii,
                         uniform VolumetricModel *uniform m)
{
  // We have to iterate till we get valid sample value
  float dt;
  float sampleVal = nan;
  while (isnan(sampleVal)) {

    // Calculate sampling step
    dt = vc.subInterval.upper - vc.subInterval.lower;

    // If the end of VKL interval has been reached
    if (dt <= 0.0f) {

      // Get next VKL interval
      VKLInterval interv;
      if (vklIterateIntervalV(ii, &interv)) {
        vc.interval = interv;

        // Calculate a new sub-interval
        const float nominalSamplingDt = interv.nominalDeltaT / m->samplingRate;
        vc.subInterval.lower = interv.tRange.lower;
        vc.subInterval.upper =
            min(interv.tRange.lower + nominalSamplingDt, interv.tRange.upper);

        // Calculate a new step
        dt = vc.subInterval.upper - vc.subInterval.lower;
      } else {

        // The end of the volume has been reached
        vc.distance = inf;
        return;
      }
    }

    // Calculate sampling distance
    vc.distance = vc.subInterval.lower + vc.jitter * dt;

    // Prepare sampling position
    const vec3f p = vc.org + vc.distance * vc.dir;

    // Sample volume value in given point
    sampleVal = vklComputeSampleV(m->volume->vklVolume,
                                  (const varying vkl_vec3f *uniform) & p);

    // Go to the next sub-interval
    const float nominalSamplingDt = vc.interval.nominalDeltaT / m->samplingRate;
    vc.subInterval.lower = vc.subInterval.upper;
    vc.subInterval.upper =
      min(vc.subInterval.lower + nominalSamplingDt, vc.interval.tRange.upper);
  }

  // Convert value into color with alpha and normalize it
  vc.sample = m->transferFunction->get(m->transferFunction, sampleVal);
  vc.sample.w *= dt * m->densityScale;
}

static float sampleAllVolumes(const VolumeIntervals &volumeIntervals,
                              varying VolumeContext *uniform volumeContexts,
                              vec4f &sampledColor)
{
  // Look for the closest sample across all volumes
  float minDist    = inf;
  int usedSampleId = -1;
  for (uniform int i = 0; i < reduce_max(volumeIntervals.numVolumeIntervals); i++) {

    // Uniform loop counter is used so we have to mask individual lanes manually
    if (i >= volumeIntervals.numVolumeIntervals)
      break;

    // Get volume data
    const varying VolumeInterval *uniform vi = volumeIntervals.intervals + i;
    VolumeContext &vc = volumeContexts[i];

    // If the sample has been used already regenerate a new one
    if (vc.ready == 0) {
      foreach_unique(ii in vc.intervalIterator) {
        foreach_unique(m in vi->volumetricModel) {
          sampleVolume(vc, ii, m);
        }
      }
      vc.ready = 1;
    }

    // Take the sample if closer
    if (vc.distance < minDist) {
      minDist      = vc.distance;
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
                               varying LDSampler *uniform ldSampler)
{
  // Uniform array of VKL iterators
  uniform VKLIntervalIterator intervalIterators[NUM_VOLUME_INTERVALS_MAX];
  uniform int iiCount = 0;

  // Array of volume contexts
  VolumeContext volumeContexts[NUM_VOLUME_INTERVALS_MAX];

  // Sampling position jitter
  const float jitter = LDSampler_getFloat(ldSampler, 0);

  // Iterate through all volumes and initialize its contexts
  for (uniform int i = 0; i < reduce_max(volumeIntervals.numVolumeIntervals); i++) {

    // Uniform loop counter is used so we have to mask individual lanes manually
    if (i >= volumeIntervals.numVolumeIntervals)
      break;

    // Transform ray into the volume local space
    const varying VolumeInterval *uniform vi = volumeIntervals.intervals + i;
    Ray transformedRay = ray;
    transformRay(transformedRay, vi->rcp_xfm);

    // Set volume context initial values
    VolumeContext &vc = volumeContexts[i];
    vc.org      = transformedRay.org;
    vc.dir      = transformedRay.dir;
    vc.jitter   = jitter;
    vc.subInterval = make_box1f(0.f, 0.f);
    vc.distance = inf;
    vc.ready    = 0;

    // There might be different volumetric models used across vector lanes
    // so we must iterate over them
    VolumetricModel *varying model = vi->volumetricModel;
    foreach_unique(m in model)
    {
      // Create volume interval interator
      uniform VKLIntervalIterator *uniform ii = &intervalIterators[iiCount++];
      vklInitIntervalIteratorV(ii,
                               m->volume->vklVolume,
                               (varying vkl_vec3f *)&transformedRay.org,
                               (varying vkl_vec3f *)&transformedRay.dir,
                               (varying vkl_range1f *)&vi->interval,
                               m->vklValueSelector);
      vc.intervalIterator = ii;
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
        volumeIntervals, volumeContexts, sampledColorOpacity);

    // Exit loop if nothing sampled
    if (dist == inf)
      break;

    // Alpha blend sampled color
    const float clampedOpacity = clamp(sampledColorOpacity.w);
    color = color + ((1.f - alpha) * clampedOpacity
      * make_vec3f(sampledColorOpacity));
    alpha = alpha + ((1.f - alpha) * clampedOpacity);
  }

  // Return final color
  return make_vec4f(color, alpha);
}

