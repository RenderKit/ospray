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
}

void Renderer_default_endFrame(uniform Renderer *uniform renderer)
{
  renderer->fb = NULL;
}


void Renderer_default_renderTile(uniform Renderer *uniform renderer,
                                     uniform Tile &tile)
{
  uniform FrameBuffer *uniform fb     = renderer->fb;
  // print("rendetile, fb=%\n",fb);
  uniform Camera      *uniform camera = renderer->camera;

  float pixel_du = .5f, pixel_dv = .5f;
  float lens_du = 0.f, lens_dv = 0.f;
  if (fb->accumID >= 0) {
    // compute 
    pixel_du = precomputedHalton2(fb->accumID);
    pixel_dv = precomputedHalton3(fb->accumID);
  }
  
  // print("du,dv %/%\n",pixel_du,pixel_dv);
  ScreenSample screenSample;
  screenSample.sampleID.z = fb->accumID;
  screenSample.z = inf;
  screenSample.alpha = 0.f;
  
  CameraSample cameraSample;
  
  for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    const uint32 frag = i*programCount+programIndex;
    screenSample.sampleID.x        = tile.region.lower.x + (frag % TILE_SIZE);
    screenSample.sampleID.y        = tile.region.lower.y + (frag / TILE_SIZE);
    if ((screenSample.sampleID.x >= fb->size.x) | (screenSample.sampleID.y >= fb->size.y)) {
      setRGBA(tile,i,make_vec3f(1,0,0));
      continue;
    }

    cameraSample.screen.x = (screenSample.sampleID.x + pixel_du) * fb->rcpSize.x;
    cameraSample.screen.y = (screenSample.sampleID.y + pixel_dv) * fb->rcpSize.y;
    
    // print("initreay %\n",camera);
    camera->initRay(camera,screenSample.ray,cameraSample);
    
    // print("rendersample\n");
    renderer->renderSample(renderer,screenSample);
    // print("write\n");
    // print("pixel % %\n",screenSample.sampleID.x,screenSample.sampleID.y);
    // if (screenSample.rgb.x > 0)
    //   print("result % % %\n",screenSample.rgb.x,screenSample.rgb.y,screenSample.rgb.z);

    setRGBAZ(tile,i,screenSample.rgb,screenSample.alpha,screenSample.z);
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
  // print("starting tile\n");
  renderer->renderTile(renderer,tile);
  // print("done tile\n");
  renderer->fb->setTile(renderer->fb,tile);
  // print("written tile\n");
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
