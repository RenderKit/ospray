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
void Renderer_default_renderSample(uniform Renderer *uniform renderer,
                                   varying ScreenSample &sample)
{
  sample.z = inf;
  sample.alpha = 1.f;
  sample.rgb = make_random_color((sample.sampleID.x<<0)+
                                 (sample.sampleID.y<<14)+
                                 (sample.sampleID.z<<28)
                                 );
}

void Renderer_default_beginFrame(uniform Renderer *uniform renderer,
                                 uniform FrameBuffer *uniform fb)
{
  renderer->fb = fb;
  if (renderer->camera == NULL)
    print("warning: ispc-side renderer % does not have a camera\n",renderer);
  if (renderer->fb == NULL)
    print("warning: ispc-side renderer % does not have a frame buffer\n",renderer);
}

void Renderer_default_endFrame(uniform Renderer *uniform renderer, 
                               const uniform int32 accumID)
{
  if (renderer->fb) renderer->fb->accumID = accumID;
  renderer->fb = NULL;
}

task void Renderer_default_renderTileJob(uniform Renderer *uniform renderer,
                                         uniform Tile &tile)
{
  uniform FrameBuffer *uniform fb     = renderer->fb;
  uniform Camera      *uniform camera = renderer->camera;

  float pixel_du = .5f, pixel_dv = .5f;
  float lens_du = 0.f,  lens_dv = 0.f;
  uniform int32 spp = renderer->spp;

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
      renderer->renderSample(renderer,screenSample);
      col = col + screenSample.rgb;
    }
    col = col * (spp_inv);
    setRGBAZ(tile,pixel,col,screenSample.alpha,screenSample.z);
  }
  // } else {
  //   if (fb->accumID >= 0) {
  //     // compute 
  //     pixel_du = precomputedHalton2(fb->accumID);
  //     pixel_dv = precomputedHalton3(fb->accumID);
  //   }
    
  //   ScreenSample screenSample;
  //   screenSample.sampleID.z = fb->accumID;
  //   screenSample.z = inf;
  //   screenSample.alpha = 0.f;

  //   CameraSample cameraSample;

  //   for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
  //     const uint32 index = i + programIndex;
  //     screenSample.sampleID.x        = tile.region.lower.x + z_order.xs[index];
  //     screenSample.sampleID.y        = tile.region.lower.y + z_order.ys[index];
  //     if ((screenSample.sampleID.x >= fb->size.x) | 
  //         (screenSample.sampleID.y >= fb->size.y)) {
  //       continue;
  //     }

  //     const uint32 pixel = z_order.xs[index] + (z_order.ys[index] * TILE_SIZE);
  //     assert(pixel < TILE_SIZE*TILE_SIZE);
  //     for (uniform uint32 s = 0; s<spp; s++) {
  //       cameraSample.screen.x = (screenSample.sampleID.x + pixel_du) * fb->rcpSize.x;
  //       cameraSample.screen.y = (screenSample.sampleID.y + pixel_dv) * fb->rcpSize.y;

  //       camera->initRay(camera,screenSample.ray,cameraSample);
  //       renderer->renderSample(renderer,screenSample);
  //       setRGBAZ(tile,pixel,screenSample.rgb,screenSample.alpha,screenSample.z);
  //     }
  // }
  // }
}

void Renderer_default_renderTile(uniform Renderer *uniform renderer,
                                 uniform Tile &tile)
{
#if 1
  precomputeZOrder();
  uniform const size_t numJobs = (TILE_SIZE*TILE_SIZE)/RENDERTILE_PIXELS_PER_JOB;
  launch[numJobs] Renderer_default_renderTileJob(renderer,tile);
#else
  uniform FrameBuffer *uniform fb     = renderer->fb;
  uniform Camera      *uniform camera = renderer->camera;

  float pixel_du = .5f, pixel_dv = .5f;
  float lens_du = 0.f,  lens_dv = 0.f;
  uniform int32 spp = renderer->spp;

  precomputeZOrder();

  if (spp > 1) {
    int startSampleID = max(fb->accumID,0)*spp;
    
    ScreenSample screenSample;
    screenSample.z = inf;
    screenSample.alpha = 0.f;
  
    CameraSample cameraSample;

    const float spp_inv = 1.f / spp;
  
    for (uint32 i=0;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
      const uint32 index = i + programIndex;
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
        renderer->renderSample(renderer,screenSample);
        col = col + screenSample.rgb;
      }
      col = col * (spp_inv);
      setRGBAZ(tile,pixel,col,screenSample.alpha,screenSample.z);
    }
  } else {
    if (fb->accumID >= 0) {
      // compute 
      pixel_du = precomputedHalton2(fb->accumID);
      pixel_dv = precomputedHalton3(fb->accumID);
    }
    
    ScreenSample screenSample;
    screenSample.sampleID.z = fb->accumID;
    screenSample.z = inf;
    screenSample.alpha = 0.f;

    CameraSample cameraSample;

    const uniform int blocks = fb->accumID > 0 || spp > 0 ? 1 : min(1 << -2 * spp, TILE_SIZE*TILE_SIZE);

    for (uint32 i=programIndex;i<TILE_SIZE*TILE_SIZE/blocks;i+=programCount) {
      screenSample.sampleID.x        = tile.region.lower.x + z_order.xs[i*blocks];
      screenSample.sampleID.y        = tile.region.lower.y + z_order.ys[i*blocks];
      if ((screenSample.sampleID.x >= fb->size.x) | 
          (screenSample.sampleID.y >= fb->size.y)) {
        continue;
      }

      cameraSample.screen.x = (screenSample.sampleID.x + pixel_du) * fb->rcpSize.x;
      cameraSample.screen.y = (screenSample.sampleID.y + pixel_dv) * fb->rcpSize.y;

      camera->initRay(camera,screenSample.ray,cameraSample);
#if 0
      if (screenSample.sampleID.x+pixel_du > 250) {
        screenSample.rgb = make_vec3f(0.f);
      } else
#endif
      renderer->renderSample(renderer,screenSample);


      for (uniform int p = 0; p < blocks; p++) {
        const uint32 pixel = z_order.xs[i*blocks+p] + (z_order.ys[i*blocks+p] * TILE_SIZE);
        assert(pixel < TILE_SIZE*TILE_SIZE);
        setRGBAZ(tile,pixel,screenSample.rgb,screenSample.alpha,screenSample.z);
      }
    }
  }
#endif
}

void Renderer_constructor(uniform Renderer *uniform renderer,
                          void *uniform cppE,
                          void *uniform _model,
                          void *uniform _camera)
{
  renderer->cppEquivalent = cppE;
  renderer->model  = (uniform Model *uniform)_model;
  renderer->camera = (uniform Camera *uniform)_camera;
  renderer->renderSample = Renderer_default_renderSample;
  renderer->renderTile   = Renderer_default_renderTile;
  renderer->beginFrame   = Renderer_default_beginFrame;
  renderer->endFrame     = Renderer_default_endFrame;
  renderer->toneMap      = NULL;
}

export void Renderer_renderTile(void *uniform _renderer,
                                uniform Tile &tile)
{
  uniform Renderer *uniform renderer = (uniform Renderer *uniform)_renderer;
  renderer->renderTile(renderer,tile);
  // renderer->fb->setTile(renderer->fb,tile);
}

export void Renderer_beginFrame(void *uniform _renderer,
                                void *uniform _fb)
{
  uniform Renderer *uniform renderer = (uniform Renderer *uniform)_renderer;
  uniform FrameBuffer *uniform fb = (uniform FrameBuffer *uniform)_fb;
  renderer->beginFrame(renderer,fb);
}


export void Renderer_endFrame(void *uniform _renderer, const uniform int32 newAccumID)
{
  uniform Renderer *uniform renderer = (uniform Renderer *uniform)_renderer;
  renderer->endFrame(renderer,newAccumID);
}

export void Renderer_setSPP(void *uniform _renderer, uniform int32 spp)
{
  uniform Renderer *uniform renderer = (uniform Renderer *uniform)_renderer;
  renderer->spp = spp;
}

export void Renderer_setNearClip(void *uniform _renderer, uniform float nearClip)
{
  uniform Renderer *uniform renderer = (uniform Renderer *uniform)_renderer;
  renderer->nearClip = nearClip;
}

export void Renderer_unproject( void *uniform _renderer,
                                const uniform vec2f &screenPos,
                                uniform bool &hit,
                                uniform float &x,
                                uniform float &y,
                                uniform float &z)
{
  uniform Renderer    *uniform renderer = (uniform Renderer *uniform)_renderer;
  uniform Camera      *uniform camera   = renderer->camera;
  uniform Model       *uniform model    = renderer->model;

  uniform OSPPickData ret;

  CameraSample cameraSample;
  cameraSample.screen.x = screenPos.x;
  cameraSample.screen.y = 1.0 - screenPos.y;

  Ray ray;
  camera->initRay(camera, ray, cameraSample);
  traceRay(model, ray);

  vec3f p = ray.org + (ray.dir * ray.t);

  hit= extract((int)(ray.geomID >= 0 || ray.instID >= 0), 0);
  x=extract(p.x,0);
  y=extract(p.y,0);
  z=extract(p.z,0);
}
