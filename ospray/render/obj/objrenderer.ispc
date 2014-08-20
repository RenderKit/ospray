/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */



// ospray stuff
#include "ospray/fb/framebuffer.ih"
#include "ospray/camera/perspectivecamera.ih"
#include "ospray/render/util.ih"
#include "ospray/common/model.ih"
#include "ospray/texture/texture2d.ih"
#include "ospray/lights/light.ih"
#include "ospray/lights/directionallight.ih"
#include "ospray/render/renderer.ih"
// obj renderer
#include "objmaterial.ih"
#include "objpointlight.ih"
#include "objspotlight.ih"

#define EPS (1e-4f)
#define ALPHA_THRESHOLD (.05f)

struct OBJRenderer {
  Renderer inherited;
  // //uniform Texture *textureArray;
  // uniform OBJMaterial *materialArray;

  const uniform OBJPointLight *uniform *uniform pointLights;
  uint32        numPointLights;
  const uniform DirectionalLight *uniform *uniform dirLights;
  uint32        numDirLights;
  const uniform OBJSpotLight *uniform *uniform spotLights;
  uint32        numSpotLights;
  vec3f         bgColor;
  bool          shadowsEnabled;
};

inline float lightAlpha(Ray &ray, uniform Model *uniform model, const float weight) {
  float alpha = 1.f;
  int max_depth = 8;
  const float org_t_max = ray.t;

  while (1) {
    traceRay(model,ray);

    if (ray.geomID < 0) return alpha;

    DifferentialGeometry dg;
    postIntersect(model, dg, ray, DG_MATERIALID | DG_TEXCOORD);

    uniform Material *material = dg.material;
    uniform OBJMaterial *objMaterial = (uniform OBJMaterial *)material;

    float material_opacity = 1;

    foreach_unique( mat in objMaterial ) {
      if (mat->map_d != NULL) {
        vec4f d_map;
        mat->map_d->get(mat->map_d, d_map, dg.st);
        material_opacity = d_map.x;
      } else if (mat->map_Kd != NULL) {
        // todo: might want to do this only if map has a alpha channel
        // (need to tag texture2d to even know that)
        vec4f kd_map;
        mat->map_Kd->get(mat->map_Kd,kd_map,dg.st);
        material_opacity = 1.f-kd_map.w;
      }
    }

    alpha = alpha * (1.f - material_opacity );

    if (alpha * weight < ALPHA_THRESHOLD) return alpha;

    max_depth--;
    if (max_depth <= 0) return alpha;

    ray.t0 = ray.t + EPS;
    ray.t = org_t_max;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }
}

inline vec3f OBJRenderer_shadeRay(varying ScreenSample &sample, const uniform OBJRenderer *uniform renderer) 
{ 
  Ray &ray = sample.ray;
  // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
  // print("ray.dir % % %\n",ray.dir.x,ray.dir.y,ray.dir.z);
  vec3f color = make_vec3f(0.f);
  float path_opacity = 1.f;
  int max_depth = 10;
  uniform int depth = 0;

  while (1) {
    ray.t0 += renderer->inherited.nearClip;
    traceRay(renderer->inherited.model,ray);

    if (ray.geomID < 0) {
      color = color + path_opacity * renderer->bgColor;
      sample.z = 1;
      return color;
    }

    sample.z = log(ray.t) / 10;

    vec3f local_shade_color = make_vec3f(0.f);

    DifferentialGeometry dg;
    postIntersect(renderer->inherited.model,
        dg,
        ray,
        DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_MATERIALID|DG_COLOR|DG_TEXCOORD);

    uniform Material *material = dg.material;
    uniform OBJMaterial *objMaterial = (uniform OBJMaterial *)material;

    if (!objMaterial) {
      color = color + path_opacity * make_vec3f(dg.color);
      return color;
    }

    float d = 1.f;
    float Ns = 0.f;
    vec3f Kd = make_vec3f(0.f,1.f,0.f);
    vec3f Ks = make_vec3f(0.f);
    vec3f bump = make_vec3f(0.f);

    foreach_unique (mat in objMaterial) {
      // should those with-map/without-map material values get
      // *multiplied* rather than used alternatively
      d = mat->map_d ? get1f(mat->map_d,dg.st) : mat->d;
      if (mat->map_Kd) {
        vec4f Kd_from_map = get4f(mat->map_Kd,dg.st);
        Kd = make_vec3f(Kd_from_map);
        d *= 1.f-Kd_from_map.w;
      } else
        Kd = mat->Kd;
      Ks = mat->map_Ks ? get3f(mat->map_Ks,dg.st,mat->Ks) : mat->Ks;
      Ns = mat->map_Ns ? get1f(mat->map_Ns,dg.st) : mat->Ns;
      bump = get3f(mat->map_Bump, dg.st, make_vec3f(0.f));
    }

    const float local_opacity = path_opacity * d;

    max_depth-=1;
    if (max_depth <= 0) return color;

    if (local_opacity > 0.01f) { //worth shading
      const vec3f R = ray.dir - (2.f * dot(ray.dir, dg.Ns))*dg.Ns;
      const vec3f P = ray.org + (ray.t * ray.dir * (1.f - EPS)) + EPS * dg.Ng;

      //Some fake hard coded ambient light
      color = color + local_opacity * Kd * .25;

      //calculate shading for point lights
      for (uniform int i = 0; i < renderer->numPointLights && renderer->pointLights; i++) {
        const uniform OBJPointLight *uniform l = (const uniform OBJPointLight *uniform)renderer->pointLights[i];
        float dist;
        const vec3f L = normalize(l->base.position - P, dist);
        if (dist > l->base.range) continue;
        const float attenuation = l->constantAttenuation
                                + l->linearAttenuation * dist
                                + l->quadraticAttenuation * dist * dist;
        const float cosNL = abs(dot(L,dg.Ns));
        const float cosLR = max(0.f,dot(L,R));
        const vec3f unshaded_light_contrib = local_opacity * (Kd * cosNL + Ks * powf(cosLR, Ns)) * l->base.color * rcpf(attenuation);
        const float max_contrib = reduce_max(unshaded_light_contrib);
        if (max_contrib > .01f) {
          Ray shadowRay;
          setRay(shadowRay,P,L);
          const float light_alpha = renderer->shadowsEnabled ? lightAlpha(shadowRay, renderer->inherited.model, max_contrib) : 1.;

          local_shade_color = local_shade_color + light_alpha * unshaded_light_contrib;
        }
      }

      //calculate shading for directional lights
      for (uniform int i = 0; i < renderer->numDirLights && renderer->dirLights; i++) {
        const uniform DirectionalLight *uniform l = (const uniform DirectionalLight *uniform)renderer->dirLights[i];
        const vec3f L = normalize(neg(l->direction));

        const float cosNL = abs(dot(L, dg.Ns));
        const float cosLR = max(0.f, dot(L,R));

        const vec3f unshaded_light_contrib = local_opacity * (Kd * cosNL + Ks * powf(cosLR, Ns)) * l->color;

        const float max_contrib = reduce_max(unshaded_light_contrib);
        if (max_contrib > .01f) {
          Ray shadowRay;
          setRay(shadowRay, P, L);
          const float light_alpha = renderer->shadowsEnabled ? lightAlpha(shadowRay, renderer->inherited.model, max_contrib) : 1.;

          local_shade_color = local_shade_color + light_alpha * unshaded_light_contrib;
        }
      }

      //calculate shading for spot lights
      for (uniform int i = 0; i < renderer->numSpotLights && renderer->spotLights; i++) {
        const uniform OBJSpotLight *uniform l = (const uniform OBJSpotLight *uniform)renderer->spotLights[i];
        float dist;
        const vec3f L = normalize(l->base.position - P, dist);
        if (dist > l->base.range) continue;
        const vec3f D = neg(l->base.direction);
        const float cosLD = dot(L,D);
        const float cosHalfAngleRads = cos((l->base.halfAngle * M_PI) / 180.0f);

        if (cosLD > cosHalfAngleRads) {  //Inside the cone
          const float cosNL = abs(dot(L, dg.Ns));
          const float cosLR = max(0.f, dot(L,R));

          const float attenuation = l->constantAttenuation
                                    + l->linearAttenuation * dist
                                    + l->quadraticAttenuation * dist * dist;

          const float angleAttenuation = (cosLD - cosHalfAngleRads) / (1 - cosHalfAngleRads);

          const vec3f unshaded_light_contrib = local_opacity * (Kd * cosNL + Ks * powf(cosLR, Ns)) * l->base.color * rcpf(attenuation) * angleAttenuation;
          const float max_contrib = reduce_max(unshaded_light_contrib);
          if (max_contrib > .01f) {
            Ray shadowRay;
            setRay(shadowRay, P, L);
            const float light_alpha = renderer->shadowsEnabled ? lightAlpha(shadowRay, renderer->inherited.model, max_contrib) : 1.;

            local_shade_color = local_shade_color + light_alpha * unshaded_light_contrib;
          }
        }
      }

      color = color + local_shade_color;
    }

    path_opacity = path_opacity * (1.f - d);
    if (path_opacity < 0.01f) return color; //remaining contribution too low kill path

    ray.t0 = ray.t + EPS;
    ray.t = infinity;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }

  return color;
}

void OBJRenderer_renderSample(uniform Renderer *uniform _renderer,
                               varying ScreenSample &sample)
{
  uniform OBJRenderer *uniform renderer = (uniform OBJRenderer *uniform)_renderer;
  // print("sample.ray.org.x %\n",sample.ray.org.x);
  // sample.rgb = absf(sample.ray.dir);
  sample.rgb = OBJRenderer_shadeRay(sample,renderer);
  // print("sample.x %\n",sample.rgb.x);
}


export void *uniform OBJRenderer_create(void *uniform cppE)
{
  uniform OBJRenderer *uniform renderer = uniform new uniform OBJRenderer;
  Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
  renderer->inherited.renderSample = OBJRenderer_renderSample;
  return renderer;
}

export void OBJRenderer_set(void *uniform _renderer,
                            void *uniform _model,
                            void *uniform _camera,
                            const uniform vec3f &bgColor,
                            const uniform bool &shadowsEnabled,
                            void **uniform pointLights,
                            uniform int32  numPointLights,
                            void **uniform dirLights,
                            uniform int32  numDirLights,
                            void **uniform spotLights,
                            uniform int32  numSpotLights)
{
  uniform OBJRenderer *uniform renderer = (uniform OBJRenderer *uniform)_renderer;
  renderer->inherited.model  = (uniform Model *uniform)_model;
  renderer->inherited.camera = (uniform Camera *uniform)_camera;

  renderer->bgColor = bgColor;

  renderer->shadowsEnabled = shadowsEnabled;

  renderer->numPointLights = numPointLights;
  renderer->pointLights = (const uniform OBJPointLight *uniform *uniform)pointLights;
  
  renderer->numDirLights = numDirLights;
  renderer->dirLights = (const uniform DirectionalLight *uniform *uniform)dirLights;

  renderer->numSpotLights = numSpotLights;
  renderer->spotLights = (const uniform OBJSpotLight *uniform *uniform)spotLights;
}


export void OBJRenderer_destroy(void *uniform _renderer)
{
  uniform OBJRenderer *uniform renderer = (uniform OBJRenderer *uniform)_renderer;
  delete renderer;
}

// export void ispc__OBJRenderer_renderTile(void *uniform _tile, //uniform Tile *uniform tile,
//                                          void *uniform _camera, //uniform Camera *uniform camera,
//                                          void *uniform _model,
//                                          uniform int num_point_lights,
//                                          void *uniform *uniform _point_lights,
//                                          uniform int num_dir_lights,
//                                          void *uniform *uniform _dir_lights)
// {
//   uniform Tile   *uniform tile   = (uniform Tile *uniform)_tile;
//   uniform Camera *uniform camera = (uniform Camera *uniform)_camera;

//   uniform OBJRenderer renderer;
//   renderer.model   = (uniform Model *uniform)_model;
//   renderer.pointLights = (const uniform OBJPointLight *uniform *uniform)_point_lights;
//   renderer.numPointLights = num_point_lights;
//   renderer.dirLights = (const uniform DirectionalLight *uniform *uniform)_dir_lights;
//   renderer.numDirLights = num_dir_lights;

//   tile->format = TILE_FORMAT_RGBA8 | TILE_FORMAT_FLOAT4;
//   const uniform uint32 size_x = tile->fbSize.x;
//   const uniform uint32 size_y = tile->fbSize.y;
//   const uniform uint32 x0 = tile->region.lower.x;
//   const uniform uint32 y0 = tile->region.lower.y;
//   for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
//     const uint32 frag = i*programCount+programIndex;
//     const uint32  x     = x0 + (frag % TILE_SIZE);
//     const uint32  y     = y0 + (frag / TILE_SIZE);
//     // print("pixel % %\n",x,y);
//     if (x < size_x & y < size_y) {
//       // print("+pixel % %\n",x,y);
//       const float screen_u = (x+.5f)/size_x;
//       const float screen_v = (y+.5f)/size_y;
//       Ray ray;
//       camera->initRay(camera,ray,make_vec2f(screen_u,screen_v),
//                       (varying vec2f)make_vec2f(0.f,0.f));
//       const vec3f col = OBJRenderer_shadeRay(ray,&renderer);
// #if 1
//       tile->r[i] = col.x;
//       tile->g[i] = col.y;
//       tile->b[i] = col.z;
//       tile->a[i] = 1.f;
//       tile->z[i] = ray.t;
//       tile->rgba8[i] = cvt_uint32(col);
// #endif
//     }
//   }
// }

