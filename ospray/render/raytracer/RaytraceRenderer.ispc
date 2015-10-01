// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "ospray/fb/FrameBuffer.ih"
#include "ospray/render/util.ih"
#include "ospray/common/Model.ih"
#include "ospray/texture/Texture2D.ih"
#include "ospray/lights/Light.ih"
#include "ospray/render/Renderer.ih"
#include "ospray/math/random.ih"
#include "ospray/math/sampling.ih"
#include "ospray/math/LinearSpace.ih"

#include "RaytraceMaterial.ih"

#define ALPHA_THRESHOLD (.05f)

struct RaytraceRenderer {
  Renderer super;

  const uniform Light *uniform *uniform lights;
  uint32 numLights;

  vec3f bgColor;
  bool  shadowsEnabled;

  int   aoSamplesPerFrame;
  float aoRayLength;
  float aoWeight;
};

inline float calculateAO(const uniform RaytraceRenderer *uniform self,
                         varying DifferentialGeometry &dg,
                         varying ScreenSample &sample)
{
  uniform int samplesPerFrame = self->aoSamplesPerFrame;
  uniform int framesPer16 = 16 / samplesPerFrame;
  int accumID = sample.sampleID.z / framesPer16;
  int sampleOfs = sample.sampleID.z % framesPer16;
  const vec2f rot = make_vec2f(precomputedHalton3(accumID),
                               precomputedHalton5(accumID));

  const int ix = sample.sampleID.x;
  const int iy = sample.sampleID.y;

  // init TEA RNG //
  uniform FrameBuffer *uniform fb = self->super.fb;
  RandomTEA rng_state;
  varying RandomTEA* const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, (fb->size.x * iy) + ix, fb->accumID);

  int hits = 0;
  const linear3f localToWorld = frame(dg.Ns);

  for (uniform int i = 0; i < samplesPerFrame; i++) {
    const vec2f s = RandomTEA__getFloats(rng);
    const vec2f rots = CranleyPattersonRotation(s, rot);
    const vec3f local_ao_dir = cosineSampleHemisphere(rots);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    Ray ao_ray;
    setRay(ao_ray, dg.P + (1e-3f * dg.Ns), ao_dir);
    ao_ray.t0 = self->super.epsilon;
    ao_ray.t  = self->aoRayLength - self->super.epsilon;
    if (dot(ao_ray.dir, dg.Ns) < 0.05f || isOccluded(self->super.model,ao_ray))
      hits++;
  }

  float frac_visible = (samplesPerFrame-hits) * (1.0f/samplesPerFrame);
  return min(.01f + 1.1f * frac_visible, 1.f);
}

inline float lightAlpha(Ray &ray, uniform Model *uniform model,
                        const float weight,
                        const uniform float epsilon)
{
  float alpha = 1.f;
  int max_depth = 8;
  const float org_t_max = ray.t;

  while (1) {
    traceRay(model,ray);

    if (ray.geomID < 0) return alpha;

    DifferentialGeometry dg;
    postIntersect(model, dg, ray, DG_MATERIALID | DG_TEXCOORD | DG_COLOR);

    uniform RaytraceMaterial *objMaterial =
        (uniform RaytraceMaterial *)dg.material;

    float material_opacity = 1.f;

    if(objMaterial == NULL) {
      material_opacity = dg.color.w;
    } else {
      foreach_unique( mat in objMaterial ) {
        material_opacity = mat->d * get1f(mat->map_d, dg.st, 1.f);
        if (mat->map_Kd) {
          vec4f Kd_from_map = get4f(mat->map_Kd,dg.st);
          material_opacity *= Kd_from_map.w;
        }
      }
    }

    alpha = alpha * (1.f - material_opacity);

    if (alpha * weight < ALPHA_THRESHOLD) return alpha;

    if (--max_depth <= 0)
      return 0.f;

    ray.t0 = ray.t + epsilon;
    ray.t = org_t_max;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }
}

inline
void RaytraceRenderer_shadeRay(const uniform RaytraceRenderer *uniform self,
                               varying ScreenSample &sample)
{ 
  Ray &ray = sample.ray;
  // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
  // print("ray.dir % % %\n",ray.dir.x,ray.dir.y,ray.dir.z);
  vec3f color = make_vec3f(0.f);
  float path_opacity = 1.f;
  int max_depth = 10;
  uniform int depth = 0;

  vec3f Ka = make_vec3f(0.25f);
  float ao = 1.0f;

  while (1) {

    traceRay(self->super.model, ray);

    // Check if we missed, if so we are done //

    if (ray.geomID < 0) {
      sample.rgb = self->super.backgroundEnabled ?
                   color + path_opacity * self->bgColor :
                   color;
      sample.alpha = self->super.backgroundEnabled ? 1.f : 1.f - path_opacity;
      sample.z = 1;
      return;
    }

    // Record ray depth //

    //! NOTE(jda) - Why was this not simply ray.t before...??
    sample.z = ray.t;//log(ray.t) / 10;

    // Start shading //

    vec3f local_shade_color = make_vec3f(0.f);

    // Calculate material information from DG //

    DifferentialGeometry dg;
    postIntersect(self->super.model,
                  dg,
                  ray,
                  DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|
                  DG_MATERIALID|DG_COLOR|DG_TEXCOORD);

    // Calculate AO contribution as ambient term //
    if (depth == 0 && self->aoSamplesPerFrame > 0 && self->aoWeight > 0.f)
    {
      const uniform int &nSamples = self->aoSamplesPerFrame;
      ao = calculateAO(self, dg, sample);
    }

    uniform Material *material = dg.material;
    uniform RaytraceMaterial *objMaterial =
        (uniform RaytraceMaterial *)material;

    float d  = 1.f;
    float Ns = 0.f;
    vec3f Kd = make_vec3f(0.f, 1.f, 0.f);
    vec3f Ks = make_vec3f(0.f);

    // Get each term from the material //

    if (!objMaterial) {
      //color = color + path_opacity * make_vec3f(dg.color);
      //return color;
      d = dg.color.w;
      Kd = make_vec3f(dg.color);
    } else {
      foreach_unique (mat in objMaterial) {
        // textures modify (mul) values, see
        //   http://paulbourke.net/dataformats/mtl/
        d = mat->d * get1f(mat->map_d, dg.st, 1.f);
        Kd = mat->Kd * make_vec3f(dg.color);
        if (mat->map_Kd) {
          vec4f Kd_from_map = get4f(mat->map_Kd, dg.st);
          Kd = Kd * make_vec3f(Kd_from_map);
          d *= Kd_from_map.w;
        }
        Ks = mat->Ks * get3f(mat->map_Ks, dg.st, make_vec3f(1.f));
        Ns = mat->Ns * get1f(mat->map_Ns, dg.st, 1.f);
//        bump = get3f(mat->map_Bump, dg.st, make_vec3f(0.f));
      }
    }

    // Blend AO into diffuse term
    Kd = (self->aoWeight * ao) * Kd + ((1.f - self->aoWeight) * Kd);

    const float local_opacity = path_opacity * d;

    max_depth -= 1;
    if (max_depth <= 0) {
      sample.rgb = color;
      sample.alpha = 1.f - path_opacity;
      return;
    }

    if (local_opacity > 0.01f) { // worth shading?
      const vec3f R = ray.dir - ((2.f * dot(ray.dir, dg.Ns)) * dg.Ns);
      const vec3f P = dg.P + self->super.epsilon * dg.Ng;

      color = color + local_opacity * Kd * Ka;

      //calculate shading for all lights
      for (uniform int i = 0; self->lights && i < self->numLights; i++) {
        const uniform Light *uniform l = self->lights[i];
        const vec2f s = make_vec2f(0.f);
        const LightSample light = l->sample(l, dg, s);

        if (reduce_max(light.radiance) > 0.f) { // any potential contribution?
          const float cosNL = abs(dot(light.direction, dg.Ns));
          const float cosLR = max(0.f, dot(light.direction, R));
          const vec3f unshaded_light_contrib = local_opacity *
              (Kd * cosNL + Ks * powf(cosLR, Ns)) * light.radiance;

          if (self->shadowsEnabled) {
            const float max_contrib = reduce_max(unshaded_light_contrib);
            if (max_contrib > .01f) {
              Ray shadowRay;
              setRay(shadowRay, P, light.direction);
              const float light_alpha = lightAlpha(shadowRay,
                                                   self->super.model,
                                                   max_contrib,
                                                   self->super.epsilon);
              local_shade_color = local_shade_color +
                                  (light_alpha * unshaded_light_contrib);
            }
          } else 
            local_shade_color = local_shade_color + unshaded_light_contrib;
        }
      }

      color = color + local_shade_color;
    }

    path_opacity = path_opacity * (1.f - d);
    if (path_opacity < 0.01f) {
      //remaining contribution too low kill path
      sample.rgb = color;
      sample.alpha = 1.f - path_opacity;
      return;
    }

    ray.t0 = ray.t + self->super.epsilon;
    ray.t = infinity;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }

  sample.rgb = color;
  sample.alpha = 1.f - path_opacity;
  return;
}

void RaytraceRenderer_renderSample(uniform Renderer *uniform _self,
                               varying ScreenSample &sample)
{
  uniform RaytraceRenderer *uniform self =
      (uniform RaytraceRenderer *uniform)_self;
  RaytraceRenderer_shadeRay(self, sample);
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void RaytraceRenderer_set(void *uniform _self,
                                 const uniform vec3f &bgColor,
                                 const uniform bool shadowsEnabled,
                                 const uniform int aoSamplesPerFrame,
                                 const uniform float aoRayLength,
                                 const uniform float aoWeight,
                                 void **uniform lights,
                                 const uniform uint32 numLights)
{
  uniform RaytraceRenderer *uniform self =
      (uniform RaytraceRenderer *uniform)_self;

  self->bgColor = bgColor;
  self->shadowsEnabled = shadowsEnabled;
  self->aoSamplesPerFrame = aoSamplesPerFrame;
  self->aoRayLength = aoRayLength;
  self->aoWeight = aoWeight;
  self->lights = (const uniform Light *uniform *uniform)lights;
  self->numLights = numLights;
}

export void *uniform RaytraceRenderer_create(void *uniform cppE)
{
  uniform RaytraceRenderer *uniform self = uniform new uniform RaytraceRenderer;
  Renderer_Constructor(&self->super,cppE);
  self->super.renderSample = RaytraceRenderer_renderSample;
  RaytraceRenderer_set(self, make_vec3f(1.f), true, 4, infinity, 1.f, NULL, 0);

  return self;
}
