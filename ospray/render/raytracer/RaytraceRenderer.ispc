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

  int   maxDepth;

  int   aoSamples;
  float aoRayLength;
  float aoWeight;
};

inline float calculateAO(const uniform RaytraceRenderer *uniform self,
                         varying DifferentialGeometry &dg,
                         varying ScreenSample &sample)
{
  const int accumID = sample.sampleID.z;
  const int ix = sample.sampleID.x;
  const int iy = sample.sampleID.y;

  // init TEA RNG //
  RandomTEA rng_state;
  varying RandomTEA* const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, (self->super.fb->size.x * iy) + ix, accumID);

  int hits = 0;
  const linear3f localToWorld = frame(dg.Ns);

  for (uniform int i = 0; i < self->aoSamples; i++) {
    const vec2f s = RandomTEA__getFloats(rng);
    const vec3f local_ao_dir = cosineSampleHemisphere(s);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    Ray ao_ray;
    setRay(ao_ray, dg.P + (1e-3f * dg.Ns), ao_dir,
           self->super.epsilon, self->aoRayLength);
    if (dot(ao_ray.dir, dg.Ns) < 0.05f || isOccluded(self->super.model,ao_ray))
      hits++;
  }

  return 1.0f - (float)hits/self->aoSamples;
}

inline float lightAlpha(Ray &ray, uniform Model *uniform model,
                        const float weight,
                        int remaining_depth,
                        const uniform float epsilon)
{
  float alpha = 1.f;
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

    if (--remaining_depth <= 0)
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
  int depth = 0;

  while (1) {

    traceRay(self->super.model, ray);

    // Check if we missed, if so we are done //

    if (ray.geomID < 0) {
      sample.rgb = self->super.backgroundEnabled ?
                   color + path_opacity * self->bgColor :
                   color;
      sample.alpha = self->super.backgroundEnabled ? 1.f : 1.f - path_opacity;
      return;
    }

    // Record depth of first hit in z buffer //
    if (depth == 0)
      sample.z = ray.t;

    // Start shading //

    // Calculate material information from DG //
    DifferentialGeometry dg;
    postIntersect(self->super.model,
                  dg,
                  ray,
                  DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|
                  DG_MATERIALID|DG_COLOR|DG_TEXCOORD);

    uniform Material *material = dg.material;
    uniform RaytraceMaterial *objMaterial =
        (uniform RaytraceMaterial *)material;

    float d  = 1.f;
    float Ns = 0.f;
    vec3f Kd = make_vec3f(0.f, 1.f, 0.f);
    vec3f Ks = make_vec3f(0.f);

    // Get each term from the material //

    if (!objMaterial) {
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

    const float local_opacity = path_opacity * d;

    if (local_opacity > 0.01f) { // worth shading?
      const vec3f R = ray.dir - ((2.f * dot(ray.dir, dg.Ns)) * dg.Ns);
      const vec3f P = dg.P + self->super.epsilon * dg.Ng;

      // Calculate AO contribution as ambient term
      if (self->aoSamples > 0 && self->aoWeight > 0.f) {
        float ao = self->aoWeight;
        if (self->aoRayLength > 0.f)
          ao *= calculateAO(self, dg, sample);
        // Blend AO w/ diffuse term
        color = color + local_opacity * Kd * ao;
      }

      //calculate shading for all lights
      for (uniform int i = 0; self->lights && i < self->numLights; i++) {
        const uniform Light *uniform l = self->lights[i];
        const vec2f s = make_vec2f(0.f);
        const LightSample light = l->sample(l, dg, s);

        if (reduce_max(light.radiance) > 0.f) { // any potential contribution?
          const float cosNL = abs(dot(light.direction, dg.Ns));
          const float cosLR = max(0.f, dot(light.direction, R));
          const vec3f brdf = Kd * cosNL + Ks * powf(cosLR, Ns);
          const vec3f light_contrib = local_opacity * brdf * light.radiance;

          if (self->shadowsEnabled) {
            const float max_contrib = reduce_max(light_contrib);
            if (max_contrib > .01f) {
              Ray shadowRay;
              setRay(shadowRay, P, light.direction);
              const float light_alpha = lightAlpha(shadowRay,
                                                   self->super.model,
                                                   max_contrib,
                                                   self->maxDepth - depth,
                                                   self->super.epsilon);
              color = color + light_alpha * light_contrib;
            }
          } else
            color = color + light_contrib;
        }
      }
    }

    // kill path when reached max depth or if remaining contribution too low 
    path_opacity = path_opacity * (1.f - d);
    depth++;
    if (depth >= self->maxDepth | path_opacity < 0.01f ) {
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
}

void RaytraceRenderer_renderSample(uniform Renderer *uniform _self,
                                   void *uniform perFrameData,
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
                                 const uniform int maxDepth,
                                 const uniform int aoSamples,
                                 const uniform float aoRayLength,
                                 const uniform float aoWeight,
                                 void **uniform lights,
                                 const uniform uint32 numLights)
{
  uniform RaytraceRenderer *uniform self =
      (uniform RaytraceRenderer *uniform)_self;

  self->bgColor = bgColor;
  self->shadowsEnabled = shadowsEnabled;
  self->maxDepth = maxDepth;
  self->aoSamples = aoSamples;
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
  RaytraceRenderer_set(self, make_vec3f(1.f), true, 10, 4, infinity, 1.f, NULL, 0);

  return self;
}
