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

  ray.t0 += jitter;

  foreach_unique(m in model)
  {
    Volume *uniform volume       = m->volume;
    TransferFunction *uniform tf = m->transferFunction;

    while (ray.t0 <= ray.t && alpha < 0.99f) {
      const float sample = volume->sample(volume, ray.org + ray.t0 * ray.dir);

      if (!isnan(sample)) {
        vec4f sampleColorOpacity = tf->get(tf, sample);

        const float clampedOpacity =
            clamp(sampleColorOpacity.w / m->samplingRate);
        color = color
            + ((1.f - alpha) * clampedOpacity * make_vec3f(sampleColorOpacity));
        alpha = alpha + ((1.f - alpha) * clampedOpacity);
      }

      volume->stepRay(volume, tf, ray, m->samplingRate);
    }
  }

  return make_vec4f(color, alpha);
}
