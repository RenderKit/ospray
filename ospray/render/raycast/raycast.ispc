/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */



// ospray stuff
#include "../../fb/framebuffer.ih"
#include "../../camera/perspectivecamera.ih"
#include "ospray/common/model.ih"
#include "../util.ih"
#include "ospray/render/renderer.ih"

struct RayCastRenderer
{
  uniform Renderer inherited;
  int shadeMode;
};

// inline vec3f shade_Ng(const Ray &ray)
// { 
//   if (ray.geomID < 0) return make_vec3f(0.f); 
//   return make_vec3f(absf(normalize(ray.Ng))); 
// }

// inline vec3f shade_primID(const Ray &ray)
// { 
//   return make_vec3f(make_random_color(ray.primID)); 
// }

// inline vec3f shade_geomID(const Ray &ray)
// { 
//   return make_vec3f(make_random_color(ray.geomID)); 
// }

// inline vec3f shade_instID(const Ray &ray)
// { 
//   return make_vec3f(make_random_color(ray.instID)); 
// }

// inline vec3f shade_eyeLight(const Ray &ray)
// { 
//   if (ray.geomID < 0) return make_vec3f(0.f); 
//   const float c = dot(normalize(ray.Ng),ray.dir); 
//   return make_vec3f(.3f+.8f*abs(c));
// }

// inline vec3f shade_eyeLight_testShadow(const Ray &ray, uniform Model *uniform model)
// { 
//   if (ray.geomID < 0) return make_vec3f(0.f); 
//   float c = dot(normalize(ray.Ng),ray.dir); 

//   Ray shadowRay;
//   const vec3f P = ray.org + (ray.t *.999f) * ray.dir;
//   const vec3f L = make_vec3f(-1,2,-.5);
//   setRay(shadowRay,P,L);
//   bool occluded = isOccluded(model,shadowRay);
//   return make_vec3f(occluded 
//                     ? .2f+.2f*abs(c)
//                     : .3f+.8f*abs(c));
// }

export void RayCastRenderer_set(void *uniform _renderer,
                                void *uniform _model,
                                void *uniform _camera)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform Camera *uniform camera = (uniform Camera *uniform)_camera;
  renderer->inherited.model = model;
  renderer->inherited.camera = camera;
}

void RayCastRenderer_renderSample_eyeLight(uniform Renderer *uniform _renderer,
                                           varying ScreenSample &sample)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;

  traceRay(renderer->inherited.model,sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0) 
    sample.rgb = make_vec3f(1.f); 
  else {
    const float c = dot(normalize(sample.ray.Ng),sample.ray.dir); 
    sample.rgb = make_vec3f(.2f+.8f*abs(c));
  }
}

void RayCastRenderer_renderSample_Ng(uniform Renderer *uniform _renderer,
                                           varying ScreenSample &sample)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;

  traceRay(renderer->inherited.model,sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0) 
    sample.rgb = make_vec3f(1.f); 
  else {
    sample.rgb = absf(sample.ray.Ng);
  }
}

void RayCastRenderer_renderSample_Ns(uniform Renderer *uniform _renderer,
                                           varying ScreenSample &sample)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;

  traceRay(renderer->inherited.model,sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0) 
    sample.rgb = make_vec3f(1.f); 
  else {
    // sample.rgb = absf(sample.ray.Ng);
    DifferentialGeometry dg;
    postIntersect(renderer->inherited.model, dg, sample.ray, DG_MATERIALID | DG_NG | DG_NS);
    sample.rgb = absf(dg.Ns);
  }
}

void RayCastRenderer_renderSample_eyeLight_primID(uniform Renderer *uniform _renderer,
                                                  varying ScreenSample &sample)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;

  traceRay(renderer->inherited.model,sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0) 
    sample.rgb = make_vec3f(0.f); 
  else {
    const float c = dot(normalize(sample.ray.Ng),sample.ray.dir); 
    sample.rgb = make_vec3f(.3f+.8f*abs(c))*make_random_color(sample.ray.primID);
  }
}

void RayCastRenderer_renderSample_eyeLight_instID(uniform Renderer *uniform _renderer,
                                                  varying ScreenSample &sample)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;

  traceRay(renderer->inherited.model,sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0) 
    sample.rgb = make_vec3f(0.f); 
  else {
    const float c = dot(normalize(sample.ray.Ng),sample.ray.dir); 
    sample.rgb = make_vec3f(.3f+.8f*abs(c))*make_random_color(sample.ray.instID);
  }
}

void RayCastRenderer_renderSample_eyeLight_geomID(uniform Renderer *uniform _renderer,
                                                  varying ScreenSample &sample)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;

  traceRay(renderer->inherited.model,sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0) 
    sample.rgb = make_vec3f(0.f); 
  else {
    const float c = dot(normalize(sample.ray.Ng),sample.ray.dir); 
    sample.rgb = make_vec3f(.3f+.8f*abs(c))*make_random_color(sample.ray.geomID);
  }
}

export void *uniform RayCastRenderer_create_eyeLight(void *uniform cppE)
{
  uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
  Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
  renderer->inherited.renderSample = RayCastRenderer_renderSample_eyeLight;
  return renderer;
}

export void *uniform RayCastRenderer_create_Ng(void *uniform cppE)
{
  uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
  Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
  renderer->inherited.renderSample = RayCastRenderer_renderSample_Ng;
  return renderer;
}

export void *uniform RayCastRenderer_create_Ns(void *uniform cppE)
{
  uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
  Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
  renderer->inherited.renderSample = RayCastRenderer_renderSample_Ns;
  return renderer;
}

export void *uniform RayCastRenderer_create_eyeLight_instID(void *uniform cppE)
{
  uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
  Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
  renderer->inherited.renderSample = RayCastRenderer_renderSample_eyeLight_instID;
  return renderer;
}

export void *uniform RayCastRenderer_create_eyeLight_geomID(void *uniform cppE)
{
  uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
  Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
  renderer->inherited.renderSample = RayCastRenderer_renderSample_eyeLight_geomID;
  return renderer;
}

export void *uniform RayCastRenderer_create_eyeLight_primID(void *uniform cppE)
{
  uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
  Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
  renderer->inherited.renderSample = RayCastRenderer_renderSample_eyeLight_primID;
  return renderer;
}

export void RayCastRenderer_destroy(void *uniform _renderer)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;
  delete renderer;
}
