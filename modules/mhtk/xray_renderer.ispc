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

#define MAX_HITS_PER_TRACE 512

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
  int32 numSwaps;
  int32 numCoherent;
};

inline vec3f make_bin_color(const int i, const int max)
{
  if (i == 0)
    return make_vec3f(0.f);
  else if (i <= max/4)
    return make_vec3f(0.f, 0.f, 1.f);
  else if (i <= max/2)
    return make_vec3f(0.f, 1.f, 0.f);
  else if (i <= 3*max/4)
    return make_vec3f(1.f, 1.f, 0.f);
  else
    return make_vec3f(1.f, 0.f, 0.f);
}

// Comment out for sorting during the intersection filter callback
#define MHTK_POST_SORTING

// Comment out to visualize average # of incoherent sorts vs. # of sort swaps
#define MHTK_SHOW_SWAPS

// Comment out to visualize sort coherence vs. ray coherence (only works if
// MHTK_SHOW_SWAPS is not defined
//#define MHTK_SHOW_RAY_COHERENCE

// /*! Intersection filter function for uniform rays. */
// typedef void (*uniform RTCFilterFuncUniform)(void* uniform ptr,    /*!< pointer to user data */
//                                      uniform RTCRay1& ray  /*!< intersection to filter */);

/*! Intersection filter function for varying rays. */
void multiHitFilter(uniform Geometry *uniform geom,
                    varying Ray& ray  /*!< intersection to filter */)
{
  if (ray.geomID < 0) return;

  varying MultiHitInfo *uniform mhi = (varying MultiHitInfo *uniform)ray.userData;

  // Save hitpoint information
#ifdef MHTK_SORTING
#  ifndef MHTK_POST_SORTING
  MHTKHit hit;
  hit.primID = ray.primID;
  hit.geomID = ray.geomID;
  hit.t      = ray.t;
  hit.Ng.x   = ray.Ng.x;
  hit.Ng.y   = ray.Ng.y;
  hit.Ng.z   = ray.Ng.z;

  int i = mhi->numHits-1;
  for (; i >= 0; --i)
  {
    if (mhi->hitArray[i].t > hit.t)
    {
      mhi->hitArray[i+1] = mhi->hitArray[i];
      mhi->numSwaps++;

#    ifndef MHTK_SHOW_RAY_COHERENCE
      /* count the number of incoherent lanes */
      foreach_unique(val in programIndex)
        mhi->numCoherent++;
#    endif
    }
    else
      break;
  }

  mhi->hitArray[i+1] = hit;
#  else
  const int pos = mhi->numHits;
  mhi->hitArray[pos].primID = ray.primID;
  mhi->hitArray[pos].geomID = ray.geomID;
  mhi->hitArray[pos].t      = ray.t;
  mhi->hitArray[pos].Ng.x   = ray.Ng.x;
  mhi->hitArray[pos].Ng.y   = ray.Ng.y;
  mhi->hitArray[pos].Ng.z   = ray.Ng.z;
#  endif
#  ifdef MHTK_SHOW_RAY_COHERENCE
      /* count the number of incoherent lanes */
      foreach_unique(val in programIndex)
        mhi->numCoherent++;
#  endif
#endif

  mhi->numHits++;

  ray.geomID = RTC_INVALID_GEOMETRY_ID; // -> discard this hit, we want them all ;-)
}

void XRayRenderer_renderSample(uniform Renderer *uniform _renderer,
                               varying ScreenSample &sample)
{
  uniform XRayRenderer *uniform renderer = (uniform XRayRenderer *uniform)_renderer;

  MultiHitInfo mhi;
  mhi.numHits     = 0;
  mhi.numSwaps    = 0;
  mhi.numCoherent = 0;

#ifdef OSPRAY_INTERSECTION_FILTER
  sample.ray.intersectionFilter = multiHitFilter;
#endif
  sample.ray.userData = &mhi;
  traceRay(renderer->inherited.model,sample.ray);

#ifdef MHTK_SORTING
# ifdef MHTK_POST_SORTING
  /* sort the hitpoints (only for measuring performance) */
  for (uniform int i = 0; i < mhi.numHits - 1; ++i)
  {
    float d = mhi.hitArray[i].t;
    int pos = i;
    bool doSwap = false;
    for (int j = i+1; j < mhi.numHits; ++j)
    {
      if(mhi.hitArray[j].t < d)
      {
        doSwap = true;
        pos = j;
        d = mhi.hitArray[j].t;
      }
    }

    if (doSwap)
    {
      varying MHTKHit tmp = mhi.hitArray[i];
      mhi.hitArray[i]     = mhi.hitArray[pos];
      mhi.hitArray[pos]   = tmp;
      mhi.numSwaps++;

#    ifndef MHTK_SHOW_RAY_COHERENCE
      /* count the number of incoherent lanes */
      foreach_unique(val in programIndex)
        mhi.numCoherent++;
#    endif
    }
  }
#  endif
#endif

  if (mhi.numHits > 0)
#if 0
    sample.rgb = make_random_color(mhi.numHits);
#else
#  ifdef MHTK_SHOW_SWAPS
    sample.rgb = make_bin_color(mhi.numSwaps, 128);
#  else
    sample.rgb = make_bin_color(programCount - (mhi.numCoherent /
                                                (float)mhi.numSwaps),
                                programCount);
#  endif
#endif
  else
    sample.rgb = make_vec3f(1);

  return;

#if 0
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
#endif
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

