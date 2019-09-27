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

vec4f integrateVolumeInterval(const ScreenSample &sample,
                              const VolumeInterval &interval,
                              Ray &ray,
                              varying LDSampler* uniform ldSampler)
{
  VolumetricModel *varying model = interval.volumetricModel;

  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  const float jitter = LDSampler_getFloat(ldSampler, 0);

  foreach_unique(m in model)
  {     
    Volume *uniform volume       = m->volume;
    TransferFunction *uniform tf = m->transferFunction;

    uniform VKLIntervalIterator intervalIterator;
    vklInitIntervalIteratorV(&intervalIterator,
                             volume->vklVolume,
                             (varying vkl_vec3f *)&ray.org,
                             (varying vkl_vec3f *)&ray.dir,
                             (varying vkl_range1f *)&interval.interval,
                             m->vklValueSelector);

    VKLInterval interval;
    while (vklIterateIntervalV(&intervalIterator, &interval) && alpha < 0.99f) {
      const float nominalSamplingDt = interval.nominalDeltaT / m->samplingRate;

      // initial sub interval, based on our renderer-defined sampling rate
      // and the volume's nominal dt
      box1f subInterval = make_box1f(
          interval.tRange.lower,
          min(interval.tRange.lower + nominalSamplingDt, interval.tRange.upper));
 
      while (subInterval.upper - subInterval.lower > 0.f && alpha < 0.99f) {
        ray.t0  = subInterval.lower + jitter * (subInterval.upper - subInterval.lower);
        const float dt = subInterval.upper - subInterval.lower;

        // Get volume sample
        vec3f p = ray.org + ray.t0 * ray.dir;
        const float sample = vklComputeSampleV(volume->vklVolume, (const varying vkl_vec3f *uniform) &p);

        if (!isnan(sample)) {
          vec4f sampleColorOpacity = tf->get(tf, sample);
 
          const float clampedOpacity =
             clamp(sampleColorOpacity.w * dt);
          color = color
              + ((1.f - alpha) * clampedOpacity * make_vec3f(sampleColorOpacity));
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
