/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */



// ospray stuff
#include "../../fb/FrameBuffer.ih"
#include "../../camera/perspectivecamera.ih"
#include "ospray/common/Model.ih"
#include "../util.ih"
#include "ospray/render/renderer.ih"

struct RayCastRenderer
{
  uniform Renderer inherited;
  int shadeMode;
};

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

//! a simple test-frame renderer that doesn't even trace a ray, just
//! returns a well-defined test frame (mostly useful for debugging
//! whether frame buffers are properly set up etcpp
void RayCastRenderer_renderSample_testFrame(uniform Renderer *uniform _renderer,
                                                       varying ScreenSample &sample)
{
  sample.rgb.x = ((sample.sampleID.x)%256)/255.f;
  sample.rgb.y = ((sample.sampleID.y)%256)/255.f;
  sample.rgb.z = ((sample.sampleID.x+sample.sampleID.y+sample.sampleID.z)%256)/255.f;
  sample.alpha = 1.f;
  sample.z = 1.f;
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

void RayCastRenderer_renderSample_eyeLight_vertexColor(uniform Renderer *uniform _renderer,
                                                       varying ScreenSample &sample)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;

  traceRay(renderer->inherited.model,sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0) 
    sample.rgb = make_vec3f(0.f); 
  else {
    // sample.rgb = absf(sample.ray.Ng);
    DifferentialGeometry dg;
    dg.color = make_vec4f(0.f);
    postIntersect(renderer->inherited.model, dg, sample.ray, DG_COLOR | DG_MATERIALID | DG_NG | DG_NS);
    sample.rgb = make_vec3f(dg.color) * abs(dot(normalize(sample.ray.dir),normalize(dg.Ns)));
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

#define DEFINE_RAYCAST_RENDERER(name)                                   \
  export void *uniform                                                  \
  RayCastRenderer_create_##name(void *uniform cppE)                     \
  {                                                                     \
    uniform RayCastRenderer *uniform renderer                           \
      = uniform new uniform RayCastRenderer;                            \
    Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);          \
    renderer->inherited.renderSample                                    \
      = RayCastRenderer_renderSample_##name;                            \
      return renderer;                                                  \
  }                                                                     \
  
DEFINE_RAYCAST_RENDERER(testFrame);
DEFINE_RAYCAST_RENDERER(eyeLight);
DEFINE_RAYCAST_RENDERER(Ng);
DEFINE_RAYCAST_RENDERER(Ns);
DEFINE_RAYCAST_RENDERER(eyeLight_primID);
DEFINE_RAYCAST_RENDERER(eyeLight_geomID);
DEFINE_RAYCAST_RENDERER(eyeLight_instID);
DEFINE_RAYCAST_RENDERER(eyeLight_vertexColor);

// export void *uniform RayCastRenderer_create_eyeLight(void *uniform cppE)
// {
//   uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
//   Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
//   renderer->inherited.renderSample = RayCastRenderer_renderSample_eyeLight;
//   return renderer;
// }

// export void *uniform RayCastRenderer_create_Ng(void *uniform cppE)
// {
//   uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
//   Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
//   renderer->inherited.renderSample = RayCastRenderer_renderSample_Ng;
//   return renderer;
// }

// export void *uniform RayCastRenderer_create_Ns(void *uniform cppE)
// {
//   uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
//   Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
//   renderer->inherited.renderSample = RayCastRenderer_renderSample_Ns;
//   return renderer;
// }

// export void *uniform RayCastRenderer_create_eyeLight_instID(void *uniform cppE)
// {
//   uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
//   Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
//   renderer->inherited.renderSample = RayCastRenderer_renderSample_eyeLight_instID;
//   return renderer;
// }

// export void *uniform RayCastRenderer_create_eyeLight_geomID(void *uniform cppE)
// {
//   uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
//   Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
//   renderer->inherited.renderSample = RayCastRenderer_renderSample_eyeLight_geomID;
//   return renderer;
// }

// export void *uniform RayCastRenderer_create_eyeLight_primID(void *uniform cppE)
// {
//   uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
//   Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
//   renderer->inherited.renderSample = RayCastRenderer_renderSample_eyeLight_primID;
//   return renderer;
// }

export void RayCastRenderer_destroy(void *uniform _renderer)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;
  delete renderer;
}
