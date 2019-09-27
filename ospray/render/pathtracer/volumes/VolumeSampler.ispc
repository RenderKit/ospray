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

#include "render/pathtracer/volumes/VolumeSampler.ih"

#include "common/World.ih"

#include "openvkl/openvkl.isph"

#define USE_INTERVAL_ITERATOR 1
#define USE_VALUE_SELECTOR 1

float delta_tracking(
  VolumetricModel *uniform vModel,
  const VolumeInterval& vInterval,
  varying RandomSampler* uniform randomSampler,
  const vec3f& o,
  const vec3f& w,
  float& mu_t,   // sampled extinction coefficint
  vec3f& albedo) // sampled albedo (color)
{
  const vec4f color_max = vModel->transferFunction->get(vModel->transferFunction, vModel->maximumDensity);
  const float mu_max = vModel->densityScale * color_max.w;
  float t = vInterval.interval.lower;
  while (true)
  {
    float xi = RandomSampler_getFloat(randomSampler);
    const float dt = - log(1.f - xi) / mu_max;
    t += dt;
    if (t > vInterval.interval.upper)
      break;
    
    xi = RandomSampler_getFloat(randomSampler);
    vec3f p = o + t * w;
    //const float sample = vModel->volume->sample(vModel->volume, p);
    const float sample = vklComputeSampleV(vModel->volume->vklVolume, (const varying vkl_vec3f *uniform) &p);
    const vec4f color = vModel->transferFunction->get(vModel->transferFunction, sample);
    mu_t = vModel->densityScale * color.w;
    if (xi < mu_t / mu_max)
    {
      albedo = make_vec3f(color);
      return t;
    }
  }
  return inf;
}

float delta_tracking_interval_homogeneous(
  VolumetricModel *uniform vModel,
  const VolumeInterval& vInterval,
  varying RandomSampler* uniform randomSampler,
  const vec3f& o,
  const vec3f& w,
  float& mu_t,   // sampled extinction coefficint
  vec3f& albedo) // sampled albedo (color)
{
  uniform VKLIntervalIterator intervalIterator;
  vklInitIntervalIteratorV(&intervalIterator,
                           vModel->volume->vklVolume,
                           (varying vkl_vec3f *)&o,
                           (varying vkl_vec3f *)&w,
                           (varying vkl_range1f *)&vInterval.interval,
#if USE_VALUE_SELECTOR==1
                           vModel->vklValueSelector
#else 
                           0
#endif
                           );
  
  float t = 0.f;
  VKLInterval interval;
  while (vklIterateIntervalV(&intervalIterator, &interval))
  {
#if USE_VALUE_SELECTOR != 1
    if (interval.valueRange.upper <= 0.f)
      continue;
#endif

    box1f subInterval = make_box1f(interval.tRange.lower, interval.tRange.upper);
    t = interval.tRange.lower;

    const float sample = 0.5f * (interval.valueRange.upper + interval.valueRange.lower);
    const vec4f color = vModel->transferFunction->get(vModel->transferFunction, sample);
    mu_t = vModel->densityScale * color.w;
    const float xi = RandomSampler_getFloat(randomSampler);
    const float dt = - log(1.f - xi) / mu_t;
    t = t + dt;
    if (t <= interval.tRange.upper) {
      albedo = make_vec3f(color);
      return t;
    }
  }
  return inf;
}

float delta_tracking_interval(
  VolumetricModel *uniform vModel,
  const VolumeInterval& vInterval,
  varying RandomSampler* uniform randomSampler,
  const vec3f& o,
  const vec3f& w,
  float& mu_t,   // sampled extinction coefficint
  vec3f& albedo) // sampled albedo (color)
{
  uniform VKLIntervalIterator intervalIterator;
  vklInitIntervalIteratorV(&intervalIterator,
                           vModel->volume->vklVolume,
                           (varying vkl_vec3f *)&o,
                           (varying vkl_vec3f *)&w,
                           (varying vkl_range1f *)&vInterval.interval,
#if USE_VALUE_SELECTOR == 1
                           vModel->vklValueSelector
#else
                           0
#endif
                           );
  
  float t = 0.f;
  VKLInterval interval;
  while (vklIterateIntervalV(&intervalIterator, &interval))
  {
#if USE_VALUE_SELECTOR != 1
    if (interval.valueRange.upper <= 0.f)
      continue;
#endif

    box1f subInterval = make_box1f(interval.tRange.lower, interval.tRange.upper);
    t = interval.tRange.lower;
    const vec4f color_max = vModel->transferFunction->get(vModel->transferFunction, interval.valueRange.upper);
    const float mu_max = vModel->densityScale * color_max.w;

    while (true)
    {
      float xi = RandomSampler_getFloat(randomSampler);
      const float dt = - log(1.f - xi) / mu_max;
      t += dt;
      if (t > interval.tRange.upper)
        break;
      
      xi = RandomSampler_getFloat(randomSampler);
      vec3f p = o + t * w;
      const float sample = vklComputeSampleV(vModel->volume->vklVolume, (const varying vkl_vec3f *uniform) &p);
      const vec4f color = vModel->transferFunction->get(vModel->transferFunction, sample);
      mu_t = vModel->densityScale * color.w;
      if (xi < mu_t / mu_max)
      {
        albedo = make_vec3f(color);
        return t;
      }
    }
  }
  return inf;
}

float volumeSampleFreePath(const PathState& pathState,
                           const World *uniform world,
                           Ray &ray,
                           varying LDSampler* uniform ldSampler,
                           varying RandomSampler* uniform randomSampler,
                           VolumetricModel* varying *uniform sampledInstance,
                           float& sampledExtinctionCoefficient,
                           vec3f& sampledAlbedo)
{
  Ray volumeRay = ray;
  VolumeIntervals volumeIntervals;
  traceVolumeRay(world, volumeRay, volumeIntervals);
  const bool haveVolumeHit = volumeIntervals.numVolumeIntervals > 0;
  float min_dist = inf;
  unmasked { *sampledInstance = NULL; }
  if (haveVolumeHit)
  {
    for (int i = 0; i < volumeIntervals.numVolumeIntervals; ++i) 
    {
      VolumeInterval vInterval = volumeIntervals.intervals[i];
      VolumetricModel *varying instance = volumeIntervals.intervals[i].volumetricModel;
      foreach_unique(inst in instance)
      {
        vInterval.interval.lower = max(0.f, vInterval.interval.lower);
        vInterval.interval.upper = noHit(ray) ? vInterval.interval.upper : min(vInterval.interval.upper, ray.t);
        if (hasInterval(vInterval))
        {
          float dist = inf;
          float extinctionCoefficient;
          vec3f albedo;
          Ray transformedVolumeRay = volumeRay;
          transformRay(transformedVolumeRay, rcp(volumeIntervals.intervals[i].xfm));
#if USE_INTERVAL_ITERATOR
          dist = delta_tracking_interval(
#else
          dist = delta_tracking(
#endif
                                inst,
                                vInterval,
                                randomSampler,
                                transformedVolumeRay.org,
                                transformedVolumeRay.dir,
                                extinctionCoefficient,
                                albedo);
          if (dist < min_dist && dist < inf) {
            min_dist = dist;
            *sampledInstance = inst;
            sampledExtinctionCoefficient = extinctionCoefficient;
            sampledAlbedo = albedo;
          }
        }
      }
    }
  }
  return min_dist;
}

float volumeTransmittance(const PathState& pathState,
                          const World *uniform world,
                          Ray &ray,
                          varying LDSampler* uniform ldSampler,
                          varying RandomSampler* uniform randomSampler)
{
  Ray volumeRay = ray;
  VolumeIntervals volumeIntervals;
  traceVolumeRay(world, volumeRay, volumeIntervals);
  const bool haveVolumeHit = volumeIntervals.numVolumeIntervals > 0;

  float transmittance = 1.f;
  if (haveVolumeHit)
  {
    for (int i = 0; i < volumeIntervals.numVolumeIntervals; ++i) 
    {
      VolumeInterval vInterval = volumeIntervals.intervals[i];
      VolumetricModel *varying instance = volumeIntervals.intervals[i].volumetricModel;
      foreach_unique(inst in instance)
      {
        vInterval.interval.lower = max(ray.t0, vInterval.interval.lower);
        vInterval.interval.upper = noHit(ray) ? vInterval.interval.upper : min(vInterval.interval.upper, ray.t);
        if (hasInterval(vInterval))
        {
          Ray transformedVolumeRay = volumeRay;
          transformRay(transformedVolumeRay, rcp(volumeIntervals.intervals[i].xfm));
          float extinctionCoefficient;
          vec3f albedo;
#if USE_INTERVAL_ITERATOR
          const float dist = delta_tracking_interval(
#else
          const float dist = delta_tracking(
#endif
              inst,
              vInterval,
              randomSampler,
              transformedVolumeRay.org,
              transformedVolumeRay.dir,
              extinctionCoefficient,
              albedo);
          transmittance = transmittance * ((dist < vInterval.interval.upper) ? 0.f : 1.f);
        }
      }
    }
  }
  return transmittance;
}
