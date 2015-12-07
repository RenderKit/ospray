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

// ospray
#include "ospray/fb/FrameBuffer.ih"
#include "ospray/camera/PerspectiveCamera.ih"
#include "ospray/common/Model.ih"
#include "ospray/render/util.ih"
#include "ospray/render/Renderer.ih"

struct Phong {
  float plastic;
  float size;
};

struct TachyonTexture {
  float ambient, diffuse, specular, opacity;
  Phong phong;
  vec3f color;
  int texFunc;
};

struct PointLight 
{
  vec3f center;
  vec3f color;
  struct {
    float constant, linear, quadratic;
  } att;
};

struct DirLight 
{
  vec3f color;
  vec3f dir;
};

struct TachyonRenderer {
  Renderer inherited;

  uniform TachyonTexture *textureArray;
  uniform PointLight *pointLight;
  uint32  numPointLights;
  uniform DirLight   *dirLight;
  uint32  numDirLights;
  bool    doShadows;
  float   lightScale;
};


#define EPS (1e-4f)
#define ALPHA_THRESHOLD (.05f)

inline float lightAlpha(Ray &ray, uniform TachyonRenderer *uniform tachyon, const float weight)
{
  if (!tachyon->doShadows)
    return 1.f; 

  float alpha = 1.f;
  int max_depth = 8; // max number of rays to be traced...
  float org_t_max = ray.t;
  while (1) {
    traceRay(tachyon->inherited.model,ray);
    if (ray.geomID < 0) 
      return alpha;
    
    DifferentialGeometry dg;
    postIntersect(tachyon->inherited.model,dg,ray,
                  DG_MATERIALID
                  );
    const float material_opacity = tachyon->textureArray[dg.materialID].opacity;
    alpha = alpha * (1.f-material_opacity);

    if (alpha * weight < ALPHA_THRESHOLD) return alpha;

    max_depth--;
    if (max_depth <= 0)
      return alpha;
    
    ray.t0 = ray.t+EPS;
    ray.t  = org_t_max;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }
}

inline vec3f shade(Ray &ray, uniform TachyonRenderer *uniform tachyon)
{ 
  vec3f color = make_vec3f(0.f);
  float path_opacity = 1.f;
  int max_depth = 10; // max number of rays to be traced...
  uniform int depth = 0;
  while (1) {
    traceRay(tachyon->inherited.model,ray);
    if (ray.geomID < 0) {
      vec3f bgColor = make_vec3f(1.f);
      // if (depth == 0)
      //   return bgColor;

      color = color + path_opacity * bgColor;
      return color;
    }
    ++depth;

    vec3f localShadeColor = make_vec3f(0.f);
    DifferentialGeometry dg;
    postIntersect(tachyon->inherited.model,dg,ray,
                  DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD
                  |DG_MATERIALID|DG_COLOR
                  );
    const float material_opacity = tachyon->textureArray[dg.materialID].opacity;
    const float local_opacity = path_opacity * material_opacity;
    
    if (local_opacity > .01f) {
      // enough weight here to shade at all ...

      TachyonTexture texture = tachyon->textureArray[dg.materialID];
      if (dg.color.x >= 0.f)
        texture.color = texture.color * make_vec3f(dg.color);

      // reflective direction, for phong highlight
      const vec3f R = ray.dir - (2.f*dot(ray.dir,dg.Ns))*dg.Ns;

      // -------------------------------------------------------
      // ambient
      // -------------------------------------------------------
      const vec3f P = ray.org+(ray.t*(ray.dir*(1.f-EPS))+EPS*dg.Ng);
      color = color + local_opacity * texture.ambient * texture.color;

      // -------------------------------------------------------
      // point lights
      // -------------------------------------------------------
      for (uniform int i=0;i<tachyon->numPointLights;i++) {
        uniform PointLight *uniform l = &tachyon->pointLight[i];
        float dist;
        const vec3f L = normalize(l->center - P,dist);
        const float attenuation
          = l->att.constant+dist*(l->att.linear+dist*l->att.quadratic);
        
        const float cosNL = abs(dot(L,dg.Ns));
        const float cosLR = max(0.f,dot(L,R));
        const float diffuse  = texture.diffuse * cosNL;
        const float specular = (texture.specular//+texture.phong.plastic
                                )* powf(cosLR,texture.phong.size);
        const vec3f unshaded_light_contrib
          = local_opacity
          * (diffuse*texture.color+make_vec3f(specular))
          * tachyon->lightScale * l->color * rcpf(attenuation);
        
        const float max_contrib = reduce_max(unshaded_light_contrib);
        if (max_contrib < .01f)
          continue;
        
        Ray shadowRay;
        setRay(shadowRay,P,L,EPS,dist*(1.f-EPS)-EPS);
        const float light_alpha = lightAlpha(shadowRay,tachyon,max_contrib);

        localShadeColor = localShadeColor + light_alpha * unshaded_light_contrib;
      }

      // -------------------------------------------------------
      // dir lights
      // -------------------------------------------------------
      for (uniform int i=0;i<tachyon->numDirLights;i++) {
        uniform DirLight *uniform l = &tachyon->dirLight[i];
        const vec3f L = normalize(neg(l->dir));
        
        const float cosNL = abs(dot(L,dg.Ns));
        const float cosLR = max(0.f,dot(L,R));
        const float diffuse  = texture.diffuse * cosNL;
        const float specular = (texture.specular+texture.phong.plastic)
          * powf(cosLR,texture.phong.size);
        const vec3f unshaded_light_contrib
          = local_opacity
          * (diffuse*texture.color+make_vec3f(specular))
          * tachyon->lightScale * l->color;
        
        const float max_contrib = reduce_max(unshaded_light_contrib);
        if (max_contrib < .01f)
          continue;
        
        Ray shadowRay;
        setRay(shadowRay,P,L,EPS,infinity);
        const float light_alpha = lightAlpha(shadowRay,tachyon,max_contrib);

        localShadeColor = localShadeColor + light_alpha * unshaded_light_contrib;
      }

      color = color + localShadeColor;
    }

    // done shading this hitpoint, determine opacity of remaing path ...
    path_opacity = path_opacity * (1.f-material_opacity);
    if (path_opacity < .01f) 
      // kill path if remaining contribution too low.
      return color;

    max_depth--;
    if (max_depth <= 0) 
      return color;

    ray.t0 = ray.t+EPS;
    ray.t  = infinity;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }

  return color;
}

void TachyonRenderer_renderSample(uniform Renderer *uniform _renderer,
                                  varying ScreenSample &sample)
{
  uniform TachyonRenderer *uniform renderer = (uniform TachyonRenderer *uniform)_renderer;
  sample.rgb = shade(sample.ray,renderer);
}

export void *uniform TachyonRenderer_create(void *uniform cppE)
{
  uniform TachyonRenderer *uniform renderer = uniform new uniform TachyonRenderer;
  Renderer_Constructor(&renderer->inherited,cppE);
  renderer->inherited.renderSample = TachyonRenderer_renderSample;
  return renderer;
}

export void TachyonRenderer_set(void *uniform _renderer,
                                void *uniform _model,
                                void *uniform _camera,
                                void *uniform _textureArray,
                                void *uniform _pointLight,
                                uniform int32  numPointLights,
                                void *uniform _dirLight,
                                uniform int32  numDirLights,
                                uniform bool   doShadows,
                                uniform float  lightScale)
{
  uniform TachyonRenderer *uniform renderer = (uniform TachyonRenderer *uniform)_renderer;
  renderer->inherited.model  = (uniform Model *uniform)_model;
  renderer->inherited.camera = (uniform Camera *uniform)_camera;

  renderer->textureArray = (uniform TachyonTexture *uniform)_textureArray;
  renderer->numPointLights = numPointLights;
  renderer->pointLight = (uniform PointLight *uniform)_pointLight;
  renderer->numDirLights = numDirLights;
  renderer->dirLight = (uniform DirLight *uniform)_dirLight;
  renderer->doShadows = doShadows;
  renderer->lightScale = lightScale;
}


