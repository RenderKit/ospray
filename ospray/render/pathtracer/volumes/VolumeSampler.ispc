// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/pathtracer/volumes/VolumeSampler.ih"

#include "common/World.ih"

#include "openvkl/openvkl.isph"

float delta_tracking(VolumetricModel *uniform vModel,
    const range1f &rInterval,
    varying RandomSampler *uniform randomSampler,
    const vec3f &o,
    const vec3f &w,
    float &mu_t, // sampled extinction coefficint
    vec3f &albedo) // sampled albedo (color)
{
  VKLIntervalIterator intervalIterator;
  vklInitIntervalIteratorV(&intervalIterator,
      vModel->volume->vklVolume,
      (varying vkl_vec3f *)&o,
      (varying vkl_vec3f *)&w,
      (varying vkl_range1f *)&rInterval,
      vModel->vklValueSelector);

  float t = 0.f;
  VKLInterval interval;
  while (vklIterateIntervalV(&intervalIterator, &interval)) {
    box1f subInterval =
        make_box1f(interval.tRange.lower, interval.tRange.upper);
    t = interval.tRange.lower;

    const float maxOpacity =
        vModel->transferFunction->getMaxOpacity(vModel->transferFunction,
            *((varying range1f * uniform) & interval.valueRange));

    const float mu_max = vModel->densityScale * maxOpacity;

    while (true) {
      float xi = RandomSampler_getFloat(randomSampler);
      const float dt = -log(1.f - xi) / mu_max;
      t += dt;
      if (t > interval.tRange.upper)
        break;

      xi = RandomSampler_getFloat(randomSampler);
      const vec3f p = o + t * w;
      const float sample = vklComputeSampleV(
          vModel->volume->vklVolume, (const varying vkl_vec3f *uniform) & p);
      const vec4f color =
          vModel->transferFunction->get(vModel->transferFunction, sample);
      mu_t = vModel->densityScale * color.w;
      if (xi < mu_t / mu_max) {
        albedo = make_vec3f(color);
        return t;
      }
    }
  }
  return inf;
}

float volumeSampleFreePath(const PathState &pathState,
    const World *uniform world,
    Ray &ray,
    RayIntervals &rayIntervals,
    varying LDSampler *uniform ldSampler,
    varying RandomSampler *uniform randomSampler,
    VolumetricModel *varying *uniform sampledInstance,
    float &sampledExtinctionCoefficient,
    vec3f &sampledAlbedo)
{
  // Get volume intervals
  Ray volumeRay = ray;
  VolumeIntervals volumeIntervals;
  traceVolumeRay(world, volumeRay, volumeIntervals);

  // Iterate through volume intervals
  float min_dist = inf;
  unmasked
  {
    *sampledInstance = NULL;
  }
  for (uniform int i = 0; i < reduce_max(volumeIntervals.numVolumeIntervals);
       i++) {
    if (i >= volumeIntervals.numVolumeIntervals)
      break;

    range1f vInterval = volumeIntervals.intervals[i].interval;
    vInterval.lower = max(0.f, vInterval.lower);
    vInterval.upper =
        noHit(ray) ? vInterval.upper : min(vInterval.upper, ray.t);

    if (isEmpty(vInterval))
      continue;

    Ray transformedVolumeRay = volumeRay;
    transformRay(transformedVolumeRay, volumeIntervals.intervals[i].rcp_xfm);

    VolumetricModel *varying instance =
        volumeIntervals.intervals[i].volumetricModel;
    foreach_unique (inst in instance) {
      for (int j = 0; j < rayIntervals.count; j++) {
        range1f rInterval = rayIntervals.intervals[j];

        // Skip ray intervals outside of the volume range
        if (or (vInterval.upper < rInterval.lower,
                rInterval.upper < vInterval.lower))
          continue;

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
            extinctionCoefficient,
            albedo);
        if (dist < inf) {
          if (dist < min_dist) {
            min_dist = dist;
            *sampledInstance = inst;
            sampledExtinctionCoefficient = extinctionCoefficient;
            sampledAlbedo = albedo;
          }
          break;
        }
      }
    }
  }
  return min_dist;
}

float volumeTransmittance(const PathState &pathState,
    const World *uniform world,
    Ray &ray,
    RayIntervals &rayIntervals,
    varying LDSampler *uniform ldSampler,
    varying RandomSampler *uniform randomSampler)
{
  // Get volume intervals
  Ray volumeRay = ray;
  VolumeIntervals volumeIntervals;
  traceVolumeRay(world, volumeRay, volumeIntervals);

  // Iterate through volume intervals
  float transmittance = 1.f;
  for (int i = 0; and(i<volumeIntervals.numVolumeIntervals, transmittance> 0.f);
       i++) {
    range1f vInterval = volumeIntervals.intervals[i].interval;
    vInterval.lower = max(ray.t0, vInterval.lower);
    vInterval.upper =
        noHit(ray) ? vInterval.upper : min(vInterval.upper, ray.t);

    if (isEmpty(vInterval))
      continue;

    Ray transformedVolumeRay = volumeRay;
    transformRay(transformedVolumeRay, volumeIntervals.intervals[i].rcp_xfm);

    VolumetricModel *varying instance =
        volumeIntervals.intervals[i].volumetricModel;
    foreach_unique (inst in instance) {
      for (int j = 0; and(j<rayIntervals.count, transmittance> 0.f); j++) {
        range1f rInterval = rayIntervals.intervals[j];

        // Skip ray intervals outside of the volume range
        if (or (vInterval.upper < rInterval.lower,
                rInterval.upper < vInterval.lower))
          continue;

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
            extinctionCoefficient,
            albedo);
        transmittance = transmittance * ((dist < rInterval.upper) ? 0.f : 1.f);
      }
    }
  }
  return transmittance;
}
