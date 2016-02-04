// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "SciVisMaterial.ih"

#define ALPHA_THRESHOLD (.05f)

// Data types
//////////////////////////////////////////////////////////////////////////////

struct SciVisRenderer
{
  Renderer base;

  const uniform Light *uniform *uniform lights;
  uint32 numLights;

  bool  shadowsEnabled;

  int   maxDepth;

  int   aoSamples;
  float aoRayLength;
  float aoWeight;
};

struct SciVisShadingInfo
{
  float d ;
  float Ns;
  vec3f Kd;
  vec3f Ks;
  float local_opacity;
  float path_opacity;
};

// Function definitions
///////////////////////////////////////////////////////////////////////////////

inline void initShadingInfo(varying SciVisShadingInfo &info)
{
  info.d  = 1.f;
  info.Ns = 0.f;
  info.Kd = make_vec3f(0.f);
  info.Ks = make_vec3f(0.f);
  info.local_opacity = 0.f;
}

inline void shadeMaterials(const varying DifferentialGeometry &dg,
                           varying SciVisShadingInfo &info)
{
  uniform SciVisMaterial *scivisMaterial =
      (uniform SciVisMaterial *)dg.material;

  // Get each term from the material //
  if (!scivisMaterial) {
    info.d = dg.color.w;
    info.Kd = make_vec3f(dg.color);
  } else {
    foreach_unique (mat in scivisMaterial) {
      // textures modify (mul) values, see
      //   http://paulbourke.net/dataformats/mtl/
      info.d = mat->d * get1f(mat->map_d, dg.st, 1.f);
      info.Kd = mat->Kd * make_vec3f(dg.color);
      if (mat->map_Kd) {
        vec4f Kd_from_map = get4f(mat->map_Kd, dg.st);
        info.Kd = info.Kd * make_vec3f(Kd_from_map);
        info.d *= Kd_from_map.w;
      }
      info.Ks = mat->Ks * get3f(mat->map_Ks, dg.st, make_vec3f(1.f));
      info.Ns = mat->Ns * get1f(mat->map_Ns, dg.st, 1.f);
    }
  }
}

inline float calculateAO(const uniform SciVisRenderer *uniform self,
                         const varying vec3i &sampleID,
                         const varying DifferentialGeometry &dg)
{
  const int accumID = sampleID.z;
  const int ix = sampleID.x;
  const int iy = sampleID.y;

  // init TEA RNG //
  RandomTEA rng_state;
  varying RandomTEA* const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, (self->base.fb->size.x * iy) + ix, accumID);

  int hits = 0;
  const linear3f localToWorld = frame(dg.Ns);

  for (uniform int i = 0; i < self->aoSamples; i++) {
    const vec2f s = RandomTEA__getFloats(rng);
    const vec3f local_ao_dir = cosineSampleHemisphere(s);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    Ray ao_ray;
    setRay(ao_ray, dg.P + (1e-3f * dg.Ns), ao_dir,
           self->base.epsilon, self->aoRayLength);
    if (dot(ao_ray.dir, dg.Ns) < 0.05f || isOccluded(self->base.model,ao_ray))
      hits++;
  }

  return 1.0f - (float)hits/self->aoSamples;
}

inline void shadeAO(const uniform SciVisRenderer *uniform self,
                    const varying vec3i &sampleID,
                    const varying DifferentialGeometry &dg,
                    const varying SciVisShadingInfo &info,
                    varying vec3f &color)
{
  // Calculate AO contribution as ambient term
  if (self->aoSamples > 0 && self->aoWeight > 0.f) {
    float ao = self->aoWeight;
    if (self->aoRayLength > 0.f)
      ao *= calculateAO(self, sampleID, dg);
    // Blend AO w/ diffuse term
    color = color + info.local_opacity * info.Kd * ao;
  }
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

    uniform SciVisMaterial *objMaterial =
        (uniform SciVisMaterial *)dg.material;

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

inline void shadeLights(const uniform SciVisRenderer *uniform self,
                        const varying Ray &ray,
                        const varying DifferentialGeometry &dg,
                        const varying SciVisShadingInfo &info,
                        const varying float depth,
                        varying vec3f &color)
{
  const vec3f R = ray.dir - ((2.f * dot(ray.dir, dg.Ns)) * dg.Ns);
  const vec3f P = dg.P + self->base.epsilon * dg.Ng;

  //calculate shading for all lights
  for (uniform int i = 0; self->lights && i < self->numLights; i++) {
    const uniform Light *uniform l = self->lights[i];
    const vec2f s = make_vec2f(0.f);
    const LightSample light = l->sample(l, dg, s);

    if (reduce_max(light.radiance) > 0.f) { // any potential contribution?
      const float cosNL = abs(dot(light.direction, dg.Ns));
      const float cosLR = max(0.f, dot(light.direction, R));
      const vec3f brdf = info.Kd * cosNL + info.Ks * powf(cosLR, info.Ns);
      const vec3f light_contrib = info.local_opacity
                                  * brdf * light.radiance;

      if (self->shadowsEnabled) {
        const float max_contrib = reduce_max(light_contrib);
        if (max_contrib > .01f) {
          Ray shadowRay;
          setRay(shadowRay, P, light.direction);
          const float light_alpha = lightAlpha(shadowRay,
                                               self->base.model,
                                               max_contrib,
                                               self->maxDepth - depth,
                                               self->base.epsilon);
          color = color + light_alpha * light_contrib;
        }
      } else
        color = color + light_contrib;
    }
  }
}

inline
void SciVisRenderer_shadeRay(const uniform SciVisRenderer *uniform self,
                             varying ScreenSample &sample)
{ 
  Ray &ray = sample.ray;
  // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
  // print("ray.dir % % %\n",ray.dir.x,ray.dir.y,ray.dir.z);
  vec3f color = make_vec3f(0.f);
  float path_opacity = 1.f;
  int depth = 0;

  while (1) {

    traceRay(self->base.model, ray);

    // Record depth of first hit in z buffer //
    if (depth == 0) {
      sample.z = ray.t;
    }

    // Check if we missed, if so we are done //

    if (ray.geomID < 0) {
      sample.rgb = self->base.backgroundEnabled ?
                   color + path_opacity * self->base.bgColor :
                   color;
      sample.alpha = 1.f - path_opacity;
      return;
    }

    // Start shading //

    // Calculate material information from DG //
    DifferentialGeometry dg;
    postIntersect(self->base.model,
                  dg,
                  ray,
                  DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|
                  DG_MATERIALID|DG_COLOR|DG_TEXCOORD);

    SciVisShadingInfo info;
    initShadingInfo(info);

    shadeMaterials(dg, info);

    info.local_opacity = path_opacity * info.d;

    if (info.local_opacity > 0.01f) { // worth shading?
      shadeAO(self, sample.sampleID, dg, info, color);
      shadeLights(self, ray, dg, info, depth, color);
    }

    // kill path when reached max depth or if remaining contribution too low 
    path_opacity = path_opacity * (1.f - info.d);
    depth++;
    if (depth >= self->maxDepth | path_opacity < 0.01f ) {
      sample.rgb = color;
      sample.alpha = 1.f - path_opacity;
      return;
    }

    ray.t0 = ray.t + self->base.epsilon;
    ray.t = infinity;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }
}

void SciVisRenderer_renderSample(uniform Renderer *uniform _self,
                                   void *uniform perFrameData,
                                   varying ScreenSample &sample)
{
  uniform SciVisRenderer *uniform self =
      (uniform SciVisRenderer *uniform)_self;
  SciVisRenderer_shadeRay(self, sample);
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void SciVisRenderer_set(void *uniform _self,
                               const uniform bool shadowsEnabled,
                               const uniform int maxDepth,
                               const uniform int aoSamples,
                               const uniform float aoRayLength,
                               const uniform float aoWeight,
                               void **uniform lights,
                               const uniform uint32 numLights)
{
  uniform SciVisRenderer *uniform self =
      (uniform SciVisRenderer *uniform)_self;

  self->shadowsEnabled = shadowsEnabled;
  self->maxDepth = maxDepth;
  self->aoSamples = aoSamples;
  self->aoRayLength = aoRayLength;
  self->aoWeight = aoWeight;
  self->lights = (const uniform Light *uniform *uniform)lights;
  self->numLights = numLights;
}

export void *uniform SciVisRenderer_create(void *uniform cppE)
{
  uniform SciVisRenderer *uniform self = uniform new uniform SciVisRenderer;
  Renderer_Constructor(&self->base,cppE);
  self->base.renderSample = SciVisRenderer_renderSample;
  SciVisRenderer_set(self, true, 10, 4, infinity, 1.f, NULL, 0);

  return self;
}
