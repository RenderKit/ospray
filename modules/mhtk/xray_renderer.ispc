/*! \file xray_renderer.ispc \brief ISPC-version of renderTile
  function for ospray::mhtk::XRayRenderer */

// ospray stuff
#include "ospray/fb/framebuffer.ih"
#include "ospray/camera/perspectivecamera.ih"
#include "ospray/common/ray.ih"
#include "ospray/render/util.ih"
#include "ospray/render/renderer.ih"
#include "ospray/common/model.ih"
#include "multihit_kernel.ih"

#define MAX_HITS_PER_TRACE 100

// inline vec4f XRayRenderer_trace(Ray &ray, RTCScene scene)
// {
//   MHTKHit hitArray[MAX_HITS_PER_TRACE];
//   const int32 numHits = multiHitKernel(scene,ray,hitArray,MAX_HITS_PER_TRACE);
// #if 0
//   return make_vec4f(make_random_color(numHits));
// #else
//   float color = 0.f;
//   float addtl = .5f;
//   for (int i=0;i<numHits;i++) {
//     vec3f N = normalize(hitArray[i].Ng);
//     color += (addtl*abs(dot(ray.dir,N)));
//     addtl *= .5f;
//   }
//   return make_vec4f(color);
// #endif
// }

struct XRayRenderer {
  uniform Renderer inherited;
  RTCScene embreeSceneHandle;
};

void XRayRenderer_renderSample(uniform Renderer *uniform _renderer,
                               varying ScreenSample &sample)
{
  uniform XRayRenderer *uniform renderer = (uniform XRayRenderer *uniform)_renderer;

  MHTKHit hitArray[MAX_HITS_PER_TRACE];
  const int32 numHits = multiHitKernel(renderer->embreeSceneHandle,
                                       sample.ray,hitArray,MAX_HITS_PER_TRACE);

  float color = 0.f;
  float addtl = .5f;
  for (int i=0;i<numHits;i++) {
    vec3f N = normalize(hitArray[i].Ng);
    color += (addtl*abs(dot(sample.ray.dir,N)));
    addtl *= .5f;
  }
  sample.rgb = make_vec3f(color);
  sample.alpha = 0.f;
}

export void *uniform XRayRenderer_create(void *uniform cppE)
{
  uniform XRayRenderer *uniform renderer = uniform new uniform XRayRenderer;
  Renderer_constructor(&renderer->inherited,cppE,NULL,NULL);
  renderer->inherited.renderSample = XRayRenderer_renderSample;
  return renderer;
}

export void XRayRenderer_set(void *uniform _renderer,
                             void *uniform _model,
                             void *uniform _camera)
{
  uniform XRayRenderer *uniform renderer = (uniform XRayRenderer *uniform)_renderer;
  uniform Model  *uniform model  = (uniform Model  *uniform)_model;
  uniform Camera *uniform camera = (uniform Camera *uniform)_camera;
  renderer->inherited.model = model;
  renderer->inherited.camera = camera;
  renderer->embreeSceneHandle = model->embreeSceneHandle;
}

