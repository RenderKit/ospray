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

void Renderer_default_endFrame(uniform Renderer *uniform renderer)
{
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

  if (spp > 1) {
    int startSampleID = max(fb->accumID,0)*spp;
    
    ScreenSample screenSample;
    screenSample.z = inf;
    screenSample.alpha = 0.f;
  
    CameraSample cameraSample;
  
    for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
      const uint32 frag = i*programCount+programIndex;
      screenSample.sampleID.x        = tile.region.lower.x + (frag % TILE_SIZE);
      screenSample.sampleID.y        = tile.region.lower.y + (frag / TILE_SIZE);
      if ((screenSample.sampleID.x >= fb->size.x) | 
          (screenSample.sampleID.y >= fb->size.y)) 
        continue;

      vec3f col = make_vec3f(0);
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
      col = col * (1.f/spp);
      setRGBAZ(tile,i,col,screenSample.alpha,screenSample.z);
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
  
    for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
      const uint32 frag = i*programCount+programIndex;
      screenSample.sampleID.x        = tile.region.lower.x + (frag % TILE_SIZE);
      screenSample.sampleID.y        = tile.region.lower.y + (frag / TILE_SIZE);
      if ((screenSample.sampleID.x >= fb->size.x) | 
          (screenSample.sampleID.y >= fb->size.y)) 
        continue;

      for (uniform uint32 s = 0; s<renderer->spp; s++) {
        cameraSample.screen.x = (screenSample.sampleID.x + pixel_du) * fb->rcpSize.x;
        cameraSample.screen.y = (screenSample.sampleID.y + pixel_dv) * fb->rcpSize.y;
      
      
        camera->initRay(camera,screenSample.ray,cameraSample);
        renderer->renderSample(renderer,screenSample);
        setRGBAZ(tile,i,screenSample.rgb,screenSample.alpha,screenSample.z);
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


export void Renderer_endFrame(void *uniform _renderer)
{
  uniform Renderer *uniform renderer = (uniform Renderer *uniform)_renderer;
  renderer->endFrame(renderer);
}

export void Renderer_setSPP(void *uniform _renderer, uniform uint32 spp)
{
  uniform Renderer *uniform renderer = (uniform Renderer *uniform)_renderer;
  renderer->spp = spp;
}
