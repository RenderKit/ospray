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

#include "Renderer.ih"
#include "ospray/render/util.ih"
#include "ospray/camera/Camera.ih"
#include "ospray/common/Model.ih"

/*! number of pixels that each job in a parallel rendertile task
    executes together. Must be a multipel of the maximum possible
    programCount (16), and must be smaller than TILE_SIZE (in one
    dimension) */
#define RENDERTILE_PIXELS_PER_JOB 128

// The layout of this struct must match that of OSPPickData in ospray.h
void Renderer_default_renderSample(uniform Renderer *uniform self,
                                   varying ScreenSample &sample)
{
  sample.z = inf;
  sample.alpha = 1.f;
  sample.rgb = make_random_color((sample.sampleID.x<<0)+
                                 (sample.sampleID.y<<14)+
                                 (sample.sampleID.z<<28)
                                 );
}

void Renderer_default_beginFrame(uniform Renderer *uniform self,
                                 uniform FrameBuffer *uniform fb)
{
  self->fb = fb;
  if (self->camera == NULL)
    print("warning: ispc-side renderer % does not have a camera\n",self);
  if (self->fb == NULL)
    print("warning: ispc-side renderer % does not have a frame buffer\n",self);
}

void Renderer_default_endFrame(uniform Renderer *uniform self, 
                               const uniform int32 accumID)
{
  if (self->fb) self->fb->accumID = accumID;
  self->fb = NULL;
}

task void Renderer_default_renderTileJob(uniform Renderer *uniform self,
                                         uniform Tile &tile)
{
  uniform FrameBuffer *uniform fb     = self->fb;
  uniform Camera      *uniform camera = self->camera;

  float pixel_du = .5f, pixel_dv = .5f;
  float lens_du = 0.f,  lens_dv = 0.f;
  uniform int32 spp = self->spp;

  // precomputeZOrder();

  // if (spp > 1) {
  const uniform int begin = taskIndex * RENDERTILE_PIXELS_PER_JOB;
  const uniform int end   = begin     + RENDERTILE_PIXELS_PER_JOB;
  const uniform int startSampleID = max(fb->accumID,0)*spp;
    
  ScreenSample screenSample;
  screenSample.z = inf;
  screenSample.alpha = 0.f;
  
  CameraSample cameraSample;

  const float spp_inv = 1.f / spp;
  
  for (uint32 index=begin+programIndex;index<end;index+=programCount) {
    // const uint32 index = i + programIndex;
    screenSample.sampleID.x        = tile.region.lower.x + z_order.xs[index];
    screenSample.sampleID.y        = tile.region.lower.y + z_order.ys[index];
    if ((screenSample.sampleID.x >= fb->size.x) | 
        (screenSample.sampleID.y >= fb->size.y)) 
      continue;

    vec3f col = make_vec3f(0.f);
    const uint32 pixel = z_order.xs[index] + (z_order.ys[index] * TILE_SIZE);
    for (uniform uint32 s = 0; s<spp; s++) {
      pixel_du = precomputedHalton2(startSampleID+s);
      pixel_dv = precomputedHalton3(startSampleID+s);
      screenSample.sampleID.z = startSampleID+s;
        
      cameraSample.screen.x = (screenSample.sampleID.x + pixel_du) * fb->rcpSize.x;
      cameraSample.screen.y = (screenSample.sampleID.y + pixel_dv) * fb->rcpSize.y;
      
      camera->initRay(camera,screenSample.ray,cameraSample);
      self->renderSample(self,screenSample);
      col = col + screenSample.rgb;
    }
    col = col * (spp_inv);
    setRGBAZ(tile,pixel,col,screenSample.alpha,screenSample.z);
  }
}

void Renderer_default_renderTile(uniform Renderer *uniform self,
                                 uniform Tile &tile)
{
// #if 1
  precomputeZOrder();
  uniform const size_t numJobs = (TILE_SIZE*TILE_SIZE)/RENDERTILE_PIXELS_PER_JOB;
  launch[numJobs] Renderer_default_renderTileJob(self,tile);
// #else
//   uniform FrameBuffer *uniform fb     = self->fb;
//   uniform Camera      *uniform camera = self->camera;

//   float pixel_du = .5f, pixel_dv = .5f;
//   float lens_du = 0.f,  lens_dv = 0.f;
//   uniform int32 spp = self->spp;

//   precomputeZOrder();

//   if (spp > 1) {
//     int startSampleID = max(fb->accumID,0)*spp;
    
//     ScreenSample screenSample;
//     screenSample.z = inf;
//     screenSample.alpha = 0.f;
  
//     CameraSample cameraSample;

//     const float spp_inv = 1.f / spp;
  
//     for (uint32 i=0;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
//       const uint32 index = i + programIndex;
//       screenSample.sampleID.x        = tile.region.lower.x + z_order.xs[index];
//       screenSample.sampleID.y        = tile.region.lower.y + z_order.ys[index];

//       if ((screenSample.sampleID.x >= fb->size.x) | 
//           (screenSample.sampleID.y >= fb->size.y)) 
//         continue;

//       vec3f col = make_vec3f(0.f);
//       const uint32 pixel = z_order.xs[index] + (z_order.ys[index] * TILE_SIZE);
//       for (uniform uint32 s = 0; s<spp; s++) {
//         pixel_du = precomputedHalton2(startSampleID+s);
//         pixel_dv = precomputedHalton3(startSampleID+s);
//         screenSample.sampleID.z = startSampleID+s;
        
//         cameraSample.screen.x = (screenSample.sampleID.x + pixel_du) * fb->rcpSize.x;
//         cameraSample.screen.y = (screenSample.sampleID.y + pixel_dv) * fb->rcpSize.y;
      
//         camera->initRay(camera,screenSample.ray,cameraSample);
//         self->renderSample(self,screenSample);
//         col = col + screenSample.rgb;
//       }
//       col = col * (spp_inv);
//       setRGBAZ(tile,pixel,col,screenSample.alpha,screenSample.z);
//     }
//   } else {
//     if (fb->accumID >= 0) {
//       // compute 
//       pixel_du = precomputedHalton2(fb->accumID);
//       pixel_dv = precomputedHalton3(fb->accumID);
//     }
    
//     ScreenSample screenSample;
//     screenSample.sampleID.z = fb->accumID;
//     screenSample.z = inf;
//     screenSample.alpha = 0.f;

//     CameraSample cameraSample;

//     const uniform int blocks = fb->accumID > 0 || spp > 0 ? 1 : min(1 << -2 * spp, TILE_SIZE*TILE_SIZE);

//     for (uint32 i=programIndex;i<TILE_SIZE*TILE_SIZE/blocks;i+=programCount) {
//       screenSample.sampleID.x        = tile.region.lower.x + z_order.xs[i*blocks];
//       screenSample.sampleID.y        = tile.region.lower.y + z_order.ys[i*blocks];
//       if ((screenSample.sampleID.x >= fb->size.x) | 
//           (screenSample.sampleID.y >= fb->size.y)) {
//         continue;
//       }

//       cameraSample.screen.x = (screenSample.sampleID.x + pixel_du) * fb->rcpSize.x;
//       cameraSample.screen.y = (screenSample.sampleID.y + pixel_dv) * fb->rcpSize.y;

//       camera->initRay(camera,screenSample.ray,cameraSample);
//       self->renderSample(self,screenSample);

//       // print("pixel % % %\n",screenSample.rgb.x,screenSample.rgb.y,screenSample.rgb.z);

//       for (uniform int p = 0; p < blocks; p++) {
//         const uint32 pixel = z_order.xs[i*blocks+p] + (z_order.ys[i*blocks+p] * TILE_SIZE);
//         assert(pixel < TILE_SIZE*TILE_SIZE);
//         setRGBAZ(tile,pixel,screenSample.rgb,screenSample.alpha,screenSample.z);
//       }
//     }
//   }
// #endif
}

void Renderer_Constructor(uniform Renderer *uniform self,
                          void *uniform cppE)
{
  self->cppEquivalent = cppE;
  self->model  = NULL;
  self->camera = NULL;
  self->fb     = NULL;
  self->spp    = 1;
  self->renderSample = Renderer_default_renderSample;
  self->renderTile   = Renderer_default_renderTile;
  self->beginFrame   = Renderer_default_beginFrame;
  self->endFrame     = Renderer_default_endFrame;
  self->toneMap      = NULL;
}

void Renderer_Constructor(uniform Renderer *uniform self,
                          void *uniform cppE,
                          void *uniform _model,
                          void *uniform _camera,
                          const uniform int32 spp)
{
  Renderer_Constructor(self, cppE);
  self->model  = (uniform Model *uniform)_model;
  self->camera = (uniform Camera *uniform)_camera;
  self->spp    = spp;
}

export void Renderer_renderTile(void *uniform _self,
                                uniform Tile &tile)
{
  uniform Renderer *uniform self = (uniform Renderer *uniform)_self;
  self->renderTile(self,tile);
}

export void Renderer_beginFrame(void *uniform _self,
                                void *uniform _fb)
{
  uniform Renderer *uniform self = (uniform Renderer *uniform)_self;
  uniform FrameBuffer *uniform fb = (uniform FrameBuffer *uniform)_fb;
  self->beginFrame(self,fb);
}


export void Renderer_endFrame(void *uniform _self, const uniform int32 newAccumID)
{
  uniform Renderer *uniform self = (uniform Renderer *uniform)_self;
  self->endFrame(self,newAccumID);
}

export void Renderer_set(void *uniform _self,
                         void *uniform _model,
                         void *uniform _camera,
                         const uniform float epsilon,
                         const uniform int32 spp)
{
  uniform Renderer *uniform self = (uniform Renderer *uniform)_self;
  self->model  = (uniform Model *uniform)_model;
  self->camera = (uniform Camera *uniform)_camera;
  self->epsilon = epsilon;
  self->spp = spp;
}

export void Renderer_pick(void *uniform _self,
                               const uniform vec2f &screenPos,
                               uniform vec3f &pos,
                               uniform bool &hit)
{
  uniform Renderer *uniform self   = (uniform Renderer *uniform)_self;
  uniform Camera   *uniform camera = self->camera;
  uniform Model    *uniform model  = self->model;

  CameraSample cameraSample;
  cameraSample.screen.x = screenPos.x;
  cameraSample.screen.y = screenPos.y;

  Ray ray;
  camera->initRay(camera, ray, cameraSample);
  traceRay(model, ray);

  vec3f p = ray.org + ray.dir * ray.t;

  pos.x = extract(p.x,0);
  pos.y = extract(p.y,0);
  pos.z = extract(p.z,0);
  hit = extract((int)(ray.geomID >= 0 || ray.instID >= 0), 0);
}
