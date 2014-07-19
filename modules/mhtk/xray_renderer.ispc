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
#include "ospray/common/ray.ih"
#include "ospray/render/util.ih"
#include "ospray/render/renderer.ih"
#include "ospray/common/model.ih"
#include "multihit_kernel.ih"

#define MAX_HITS_PER_TRACE 2

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

struct MultiHitInfo {
  MHTKHit hitArray[MAX_HITS_PER_TRACE];
  int32 numHits;
};


// /*! Intersection filter function for uniform rays. */
// typedef void (*uniform RTCFilterFuncUniform)(void* uniform ptr,    /*!< pointer to user data */
//                                      uniform RTCRay1& ray  /*!< intersection to filter */);

/*! Intersection filter function for varying rays. */
void multiHitFilter(uniform Geometry *uniform geom,
                    varying Ray& ray  /*!< intersection to filter */)
{
  if (ray.geomID < 0) return;

  varying MultiHitInfo *uniform mhi = (varying MultiHitInfo *uniform)ray.userData;

  mhi->numHits++;

  ray.geomID = -1; // -> discard this hit, we want them all ;-)
}

void XRayRenderer_renderSample(uniform Renderer *uniform _renderer,
                               varying ScreenSample &sample)
{
  uniform XRayRenderer *uniform renderer = (uniform XRayRenderer *uniform)_renderer;

  MultiHitInfo mhi;
  mhi.numHits = 0;

#ifdef OSPRAY_INTERSECTION_FILTER
  sample.ray.intersectionFilter = multiHitFilter;
#endif
  sample.ray.userData = &mhi;
  traceRay(renderer->inherited.model,sample.ray);


  sample.rgb = make_random_color(mhi.numHits);
  return;
  // MHTKHit hitArray[MAX_HITS_PER_TRACE];
  // const int32 numHits = multiHitKernel(renderer->embreeSceneHandle,
  //                                      sample.ray,hitArray,MAX_HITS_PER_TRACE);
  // print("num %\n",mhi.numHits);

  float color = 0.f;
  float addtl = .5f;
  for (int i=0;i<mhi.numHits;i++) {
    vec3f N = normalize(mhi.hitArray[i].Ng);
    color += (addtl*abs(dot(sample.ray.dir,N)));
    addtl *= .5f;
  }
   // print("col % % %\n",
   //       sample.rgb.x,
   //       sample.rgb.y,
   //       sample.rgb.z
   //       );
  sample.rgb = make_vec3f(color);
  sample.alpha = 0.f;
}

export void *uniform XRayRenderer_create(void *uniform cppE)
{
#ifndef OSPRAY_INTERSECTION_FILTER
  print("MultiHit XRay-Renderer REQUIRES OSPRAY_INTERSECTION_FILTER TO BE TURNED ON!\n");
  return NULL;
#endif
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

