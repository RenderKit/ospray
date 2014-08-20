/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "renderer.ih"

#include "renderer.ih"
#include "ospray/render/util.ih"
#include "ospray/camera/camera.ih"

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


void Renderer_default_renderTile(uniform Renderer *uniform renderer,
                                 uniform Tile &tile)
{
  uniform FrameBuffer *uniform fb     = renderer->fb;
  uniform Camera      *uniform camera = renderer->camera;

  float pixel_du = .5f, pixel_dv = .5f;
  float lens_du = 0.f, lens_dv = 0.f;
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

      vec3f col = make_vec3f(0);
      const uint32 pixel = z_order.xs[index] + (z_order.ys[index] * TILE_SIZE);
      for (uniform uint32 s = 0; s<renderer->spp; s++) {
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

    for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
      const uint32 index = i + programIndex;
      screenSample.sampleID.x        = tile.region.lower.x + z_order.xs[index];
      screenSample.sampleID.y        = tile.region.lower.y + z_order.ys[index];
      //print("index          %\n", index);
      //print("screenSample   %, %\n", screenSample.sampleID.x, screenSample.sampleID.y);
      //print("offset         %, %\n", z_order.xs[index], z_order.ys[index]);
      //print("tile region    %, %\n", tile.region.lower.x, tile.region.lower.y);
      if ((screenSample.sampleID.x >= fb->size.x) | 
          (screenSample.sampleID.y >= fb->size.y)) {
        //print("Oversized! %,% >= %,%\n", screenSample.sampleID.x, screenSample.sampleID.y, fb->size.x, fb->size.y);
        continue;
      }

      const uint32 pixel = z_order.xs[index] + (z_order.ys[index] * TILE_SIZE);
      //print("linear offset  %+{%*%}=%\n", z_order.xs[index], z_order.ys[index], TILE_SIZE, pixel);
      //print("\n");
      assert(pixel < TILE_SIZE*TILE_SIZE);
      for (uniform uint32 s = 0; s<renderer->spp; s++) {
        cameraSample.screen.x = (screenSample.sampleID.x + pixel_du) * fb->rcpSize.x;
        cameraSample.screen.y = (screenSample.sampleID.y + pixel_dv) * fb->rcpSize.y;

        // print("initray % % %\n",camera,cameraSample.screen.x,cameraSample.screen.y);
        camera->initRay(camera,screenSample.ray,cameraSample);
        // if (any(screenSample.sampleID.y == 128 && screenSample.sampleID.x == 512)) {
        //   print("screen.x %\n",cameraSample.screen.x);
        //   print("screen.y %\n",cameraSample.screen.y);
        //   print("ray.org.x %\n",screenSample.ray.org.x);
        //   print("ray.org.y %\n",screenSample.ray.org.y);
        //   print("ray.org.z %\n",screenSample.ray.org.z);
        //   print("ray.dir.x %\n",screenSample.ray.dir.x);
        //   print("ray.dir.y %\n",screenSample.ray.dir.y);
        //   print("ray.dir.z %\n",screenSample.ray.dir.z);
        //   print("ray.t0 %\n",screenSample.ray.t0);
        //   print("ray.t %\n",screenSample.ray.t);
        //   screenSample.ray.Ng.x = 1.f;
        // } else {
        //   screenSample.ray.Ng.x = 0.f;
        // }

        // print("rendersample %\n",renderer);
        renderer->renderSample(renderer,screenSample);
        // print("setrgba % %\n",&tile,i);
        // if (screenSample.rgb.x != 0) {
        //   print("r %\n",screenSample.rgb.x);
        //   print("g %\n",screenSample.rgb.y);
        //   print("b %\n",screenSample.rgb.z);
        // }
        // if (any(screenSample.sampleID.y == 128 && screenSample.sampleID.x == 512)) {
        //   print("ray.org.x %\n",screenSample.ray.org.x);
        //   print("ray.org.y %\n",screenSample.ray.org.y);
        //   print("ray.org.z %\n",screenSample.ray.org.z);
        //   print("ray.dir.x %\n",screenSample.ray.dir.x);
        //   print("ray.dir.y %\n",screenSample.ray.dir.y);
        //   print("ray.dir.z %\n",screenSample.ray.dir.z);
        //   print("ray.t %\n",screenSample.ray.t);
        //   print("ray.primID %\n",screenSample.ray.primID);
        //   print("ray.geomID %\n",screenSample.ray.geomID);
        // print("rgb %\n",screenSample.rgb.x);
        // }

        //vec3f p = make_vec3f(((float)index)/(TILE_SIZE*TILE_SIZE));
        setRGBAZ(tile,pixel,screenSample.rgb,screenSample.alpha,screenSample.z);
        // print("DONE setrgba % %\n",&tile,i);
      }
    }
  }
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
  renderer->fb->setTile(renderer->fb,tile);
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

export void Renderer_setSPP(void *uniform _renderer, uniform uint32 spp)
{
  uniform Renderer *uniform renderer = (uniform Renderer *uniform)_renderer;
  renderer->spp = spp;
}

export void Renderer_setNearClip(void *uniform _renderer, uniform float nearClip)
{
  uniform Renderer *uniform renderer = (uniform Renderer *uniform)_renderer;
  renderer->nearClip = nearClip;
}
