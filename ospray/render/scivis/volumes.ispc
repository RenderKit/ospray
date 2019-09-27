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

#define USE_VALUE_SELECTOR 1

vec4f integrateVolumeInterval(const ScreenSample &sample,
                              const VolumeInterval &interval,
                              Ray &ray)
{
  VolumetricModel *varying model = interval.volumetricModel;

  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  // Ray offset for this sample, as a fraction of the nominal step size.
  float jitter = precomputedHalton2(sample.sampleID.z);
  int ix       = sample.sampleID.x % 4;
  int iy       = sample.sampleID.y % 4;

  int patternID = ix + 4 * iy;
  jitter += precomputedHalton3(patternID);

  if (jitter > 1.f)
    jitter -= 1.f;

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
                             //&tRange,
#if USE_VALUE_SELECTOR==1
                             m->vklValueSelector
#else
                             0
#endif
                             );

    VKLInterval interval;
    while (vklIterateIntervalV(&intervalIterator, &interval) && alpha < 0.99f) {

#if USE_VALUE_SELECTOR != 1
      if (interval.valueRange.upper <= 0.f)
        continue;
#endif

      const float nominalSamplingDt = interval.nominalDeltaT / m->samplingRate;

      // initial sub interval, based on our renderer-defined sampling rate
      // and the volume's nominal dt
      box1f subInterval = make_box1f(
          interval.tRange.lower,
          min(interval.tRange.lower + nominalSamplingDt, interval.tRange.upper));
 
      while (subInterval.upper - subInterval.lower > 0.f && alpha < 0.99f) {
        ray.t0  = 0.5f * (subInterval.lower + subInterval.upper) + jitter;
        const float dt = subInterval.upper - subInterval.lower;

        // Get volume sample
        vec3f p = ray.org + ray.t0 * ray.dir;
        const float sample = vklComputeSampleV(volume->vklVolume, (const varying vkl_vec3f *uniform) &p);

        if (!isnan(sample)) {
          vec4f sampleColorOpacity = tf->get(tf, sample);
 
          const float clampedOpacity =
             clamp(sampleColorOpacity.w / m->samplingRate);
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
