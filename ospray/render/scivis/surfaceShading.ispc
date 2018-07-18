// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "SciVisMaterial.ih"
#include "math/random.ih"
#include "math/sampling.ih"

void initShadingInfo(varying SciVisShadingInfo &info)
{
  info.d  = 1.f;
  info.Ns = 0.f;
  info.Kd = make_vec3f(0.f);
  info.Ks = make_vec3f(0.f);
  info.albedo = make_vec3f(0.f);

  info.local_opacity = 0.f;
}

// Material functions //

void shadeMaterials(const varying DifferentialGeometry &dg,
                    varying SciVisShadingInfo &info)
{
  SciVisMaterial *scivisMaterial = (SciVisMaterial *)dg.material;

  info.shadingNormal = dg.Ns;

  // Get each term from the material //
  if (!scivisMaterial) {
    info.d = dg.color.w;
    info.Kd = make_vec3f(dg.color);
  } else {
    foreach_unique (mat in scivisMaterial) {
      // textures modify (mul) values, see
      //   http://paulbourke.net/dataformats/mtl/
      info.d = mat->d * get1f(mat->map_d, dg, 1.f) * dg.color.w;
      info.Kd = mat->Kd * make_vec3f(dg.color);
      if (valid(mat->map_Kd) ){
        vec4f Kd_from_map = get4f(mat->map_Kd, dg);
        info.Kd = info.Kd * make_vec3f(Kd_from_map);
        info.d *= Kd_from_map.w;
      }
      info.Ks = mat->Ks * get3f(mat->map_Ks, dg, make_vec3f(1.f));
      info.Ns = mat->Ns * get1f(mat->map_Ns, dg, 1.f);
      // normal mapping
      if (valid(mat->map_Bump)) {
        // get normal from texture
        vec3f localNormal = getNormal(mat->map_Bump, dg);
        vec2f rotNormal = mat->rot_Bump * make_vec2f(localNormal.x, localNormal.y);
        localNormal.x = rotNormal.x; localNormal.y = rotNormal.y;
        // transform to world space and align to tangents/texture coordinates
        linear3f f = make_LinearSpace3f(normalize(dg.dPds), normalize(dg.dPdt), dg.Ns);
        // in general f is not ortho-normal, thus need to re-normalize
        info.shadingNormal = normalize(f * localNormal);
      }
    }
  }

  info.albedo = info.d * (info.Kd + info.Ks);

  // convert from Phong exponent to Blinn
  info.Ns *= 4.0f;
  // BRDF normalization
  info.Kd = info.Kd * one_over_pi;
  info.Ks = info.Ks * ((info.Ns + 2.f) * one_over_four_pi *
      rcp(2.f - exp(info.Ns * -lntwo_over_two)));
}

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
  RandomTEA__Constructor(rng, 0x290374, (self->super.fb->size.x * iy) + ix);
  const vec2f rot = RandomTEA__getFloats(rng);

  float occlusion = 0.f;
  const linear3f localToWorld = frame(shadingNormal);

  for (uniform int i = 0; i < self->aoSamples; i++) {
    const vec2f halton = HaltonSequence_get2D(accumID * self->aoSamples + i);
    const vec2f r = CranleyPattersonRotation(halton, rot);
    const vec3f local_ao_dir = cosineSampleHemisphere(r);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    if (dot(ao_dir, dg.Ns) < 0.05f) { // check below surface
      occlusion += 1.f;
      continue;
    }

    Ray ao_ray;
    setRay(ao_ray, dg.P + (self->super.epsilon * dg.Ns), ao_dir,
           self->super.epsilon, self->aoDistance);

    if (self->aoTransparencyEnabled) {
      const float rayOffset = self->super.epsilon*(1.f + r.x);
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
      const vec3f H = normalize(light.dir - ray.dir);
      const float cosNH = dot(H, info.shadingNormal);
      const vec3f specular = info.Ks * powf(cosNH, info.Ns);
      const vec3f diffuse = info.Kd * cosNL;
      const vec3f light_contrib =
        info.local_opacity * (diffuse + specular) * light.weight;

      if (self->shadowsEnabled) {
        const float max_contrib = reduce_max(light_contrib);
        if (max_contrib > self->super.minContribution) {
          Ray shadowRay;
          setRay(shadowRay, P, light.dir, self->super.epsilon,
              light.dist - self->super.epsilon, ray.time);
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

