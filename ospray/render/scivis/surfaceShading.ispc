// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "surfaceShading.ih"
#include "math/random.ih"
#include "math/sampling.ih"

// AO functions //

float calculateAO(const uniform SciVisRenderer *uniform self,
                  const varying vec3i &sampleID,
                  const varying DifferentialGeometry &dg,
                  const varying vec3f &shadingNormal)
{
  const int accumID = sampleID.z;
  const int ix = sampleID.x;
  const int iy = sampleID.y;

  // init TEA RNG //
  RandomTEA rng_state;
  varying RandomTEA* const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, (self->super.fb->size.x * iy) + ix, accumID);

  float occlusion = 0.f;
  const linear3f localToWorld = frame(shadingNormal);

  for (uniform int i = 0; i < self->aoSamples; i++) {
    const vec2f s = RandomTEA__getFloats(rng);
    const vec3f local_ao_dir = cosineSampleHemisphere(s);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    if (dot(ao_dir, dg.Ns) < 0.05f) { // check below surface
      occlusion += 1.f;
      continue;
    }

    Ray ao_ray;
    setRay(ao_ray, dg.P + (self->super.epsilon * dg.Ns), ao_dir,
           self->super.epsilon, self->aoDistance);

    if (self->aoTransparencyEnabled) {
      const float rayOffset = self->super.epsilon*(1.f + s.x);
      occlusion += (1.f - lightAlpha(self, ao_ray, self->super.model, 1.0f,
                                     rayOffset, sampleID, 0.2f));
    } else if (isOccluded(self->super.model, ao_ray))
      occlusion += 1.f;
  }

  // the cosTheta of cosineSampleHemispherePDF and dot(shadingNormal, ao_dir) cancel
  return 1.0f - occlusion/self->aoSamples;
}


void shadeAO(const uniform SciVisRenderer *uniform self,
             const varying vec3i &sampleID,
             const varying DifferentialGeometry &dg,
             const varying SciVisShadingInfo &info,
             varying vec3f &color)
{
  // Calculate AO contribution as ambient term
  float ao = 1.0f;
  if (self->needToDoAO)
    ao = calculateAO(self, sampleID, dg, info.shadingNormal);
  
  color = color + (info.local_opacity * ao) * info.Kd * self->aoColor;
}

void integrateOverLights(const uniform SciVisRenderer *uniform self,
                         const varying Ray &ray,
                         const varying DifferentialGeometry &dg,
                         const varying SciVisShadingInfo &info,
                         varying vec3f &color,
                         const varying float &rayOffset,
                         const varying vec3i &sampleID,
                         const uniform float quality)
{
  const vec3f R = ray.dir - ((2.f * dot(ray.dir, info.shadingNormal)) * info.shadingNormal);
  const vec3f P = dg.P + self->super.epsilon * dg.Ng;

  //calculate shading for all lights
  for (uniform int i = 0; self->lights && i < self->numLights; i++) {
    const uniform Light *uniform l = self->lights[i];
    const vec2f s = make_vec2f(0.0f); // sample center of area lights
    const Light_SampleRes light = l->sample(l, dg, s);

    if (reduce_max(light.weight) > 0.f) { // any potential contribution?
      float cosNL = dot(light.dir, info.shadingNormal);
      if (self->oneSidedLighting) {
        if (cosNL < 0.0f)
          continue;
      } else
        cosNL = abs(cosNL);
      const float cosLR = max(0.f, dot(light.dir, R));
      vec3f specular = info.Ks * powf(cosLR, info.Ns);
      const vec3f brdf = info.Kd + specular;
      const vec3f light_contrib = info.local_opacity * brdf * cosNL * light.weight;

      if (self->shadowsEnabled) {
        const float max_contrib = reduce_max(light_contrib);
        if (max_contrib > .01f) {
          Ray shadowRay;
          setRay(shadowRay, P, light.dir);
          const float light_alpha = lightAlpha(self, shadowRay,
              self->super.model,
              max_contrib,
              rayOffset, sampleID, quality);
          color = color + light_alpha * light_contrib;
        }
      } else {
        color = color + light_contrib;
      }
    }
  }
}

