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

float delta_tracking(
  VolumetricModel *uniform vModel,
  const VolumeInterval& vInterval,
  varying RandomSampler* uniform randomSampler,
  const vec3f& o,
  const vec3f& w,
  float& mu_t,   // sampled extinction coefficint
  vec3f& albedo) // sampled albedo (color)
{
  float mu_max = vModel->densityScale * vModel->maximumDensity;
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
    const float sample = vModel->volume->sample(vModel->volume, p);
    const vec4f color =
        vModel->transferFunction->get(vModel->transferFunction, sample);
    mu_t = vModel->densityScale * color.w;
    if (xi < mu_t / mu_max)
    {
      albedo = make_vec3f(color);
      return t;
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
          if (pathState.homogeneousApprox)
          {
            float density = inst->densityScale;
            dist = vInterval.interval.lower;
            float xi = RandomSampler_getFloat(randomSampler);
            const float dt = - log(1.f - xi) / density;
            dist += dt;
            if (dist > vInterval.interval.upper)
              dist = inf;
          }
          else 
          {
            Ray transformedVolumeRay = volumeRay;
            transformRay(transformedVolumeRay, rcp(volumeIntervals.intervals[i].xfm));
            dist = delta_tracking(inst,
                                  vInterval,
                                  randomSampler,
                                  transformedVolumeRay.org,
                                  transformedVolumeRay.dir,
                                  extinctionCoefficient,
                                  albedo);
          }
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
          if (pathState.homogeneousApprox) {
            transmittance = transmittance * exp(-(vInterval.interval.upper - vInterval.interval.lower)*inst->densityScale);
          } else {
            Ray transformedVolumeRay = volumeRay;
            transformRay(transformedVolumeRay, rcp(volumeIntervals.intervals[i].xfm));
            float extinctionCoefficient;
            vec3f albedo;
            const float dist = delta_tracking(
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
  }
  return transmittance;
}
