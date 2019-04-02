// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
#include "camera/Camera.ih"
#include "common/World.ih"
#include "render/util.ih"

// The layout of this struct must match that of OSPPickData in ospray.h
void Renderer_default_renderSample(Renderer *uniform self,
                                   FrameBuffer *uniform fb,
                                   World *uniform model,
                                   void *uniform perFrameData,
                                   varying ScreenSample &sample)
{
  sample.z     = inf;
  sample.alpha = 1.f;
  sample.rgb =
      make_random_color((sample.sampleID.x << 0) + (sample.sampleID.y << 14) +
                        (sample.sampleID.z << 28));
}

static unmasked void *uniform
Renderer_default_beginFrame(Renderer *uniform self, const World *uniform world)
{
  return NULL;
}

static unmasked void Renderer_default_endFrame(Renderer *uniform self,
                                               void *uniform perFrameData)
{
}

unmasked void Renderer_default_renderTile(Renderer *uniform self,
                                          FrameBuffer *uniform fb,
                                          Camera *uniform camera,
                                          World *uniform world,
                                          void *uniform perFrameData,
                                          uniform Tile &tile,
                                          uniform int taskIndex)
{
  const uniform int32 spp = self->spp;

  ScreenSample screenSample;
  screenSample.z     = inf;
  screenSample.alpha = 0.f;

  CameraSample cameraSample;

  const uniform int begin         = taskIndex * RENDERTILE_PIXELS_PER_JOB;
  const uniform int end           = begin + RENDERTILE_PIXELS_PER_JOB;
  const uniform int startSampleID = max(tile.accumID, 0) * spp;

  for (uniform uint32 i = begin; i < end; i += programCount) {
    const uint32 index      = i + programIndex;
    screenSample.sampleID.x = tile.region.lower.x + z_order.xs[index];
    screenSample.sampleID.y = tile.region.lower.y + z_order.ys[index];

    if ((screenSample.sampleID.x >= fb->size.x) |
        (screenSample.sampleID.y >= fb->size.y))
      continue;

    float tMax = inf;
    // set ray t value for early ray termination if we have a maximum depth
    // texture
    if (self->maxDepthTexture) {
      // always sample center of pixel
      vec2f depthTexCoord;
      depthTexCoord.x = (screenSample.sampleID.x + 0.5f) * fb->rcpSize.x;
      depthTexCoord.y = (screenSample.sampleID.y + 0.5f) * fb->rcpSize.y;

      DifferentialGeometry lookup;
      initDgFromTexCoord(lookup, depthTexCoord);
      tMax = min(get1f(self->maxDepthTexture, lookup), inf);
    }
    vec3f col    = make_vec3f(0.f);
    float alpha  = 0.f;
    vec3f normal = make_vec3f(0.f);
    vec3f albedo = make_vec3f(0.f);

    const uint32 pixel = z_order.xs[index] + (z_order.ys[index] * TILE_SIZE);

    for (uniform uint32 s = 0; s < spp; s++) {
      const float pixel_du    = precomputedHalton2(startSampleID + s);
      const float pixel_dv    = precomputedHalton3(startSampleID + s);
      screenSample.sampleID.z = startSampleID + s;

      cameraSample.screen.x =
          (screenSample.sampleID.x + pixel_du) * fb->rcpSize.x;
      cameraSample.screen.y =
          (screenSample.sampleID.y + pixel_dv) * fb->rcpSize.y;

      // TODO: fix correlations / better RNG
      cameraSample.lens.x = precomputedHalton3(startSampleID + s);
      cameraSample.lens.y = precomputedHalton5(startSampleID + s);

      camera->initRay(camera, screenSample.ray, cameraSample);
      screenSample.ray.t = min(screenSample.ray.t, tMax);

      self->renderSample(self, fb, world, perFrameData, screenSample);
      col = col + screenSample.rgb;
      alpha += screenSample.alpha;
      normal = normal + screenSample.normal;
      albedo = albedo + screenSample.albedo;
    }
    const float rspp    = rcpf(spp);
    screenSample.rgb    = col * rspp;
    screenSample.alpha  = alpha * rspp;
    screenSample.normal = normal * rspp;
    screenSample.albedo = albedo * rspp;
    setTile(tile, pixel, screenSample);
  }
}

export void Renderer_set(void *uniform _self,
                         const uniform int32 spp,
                         const uniform int32 maxDepth,
                         const uniform float minContribution,
                         const uniform vec4f &bgColor,
                         void *uniform _maxDepthTexture);

void Renderer_Constructor(uniform Renderer *uniform self, void *uniform cppE)
{
  self->cppEquivalent = cppE;
  self->renderSample  = Renderer_default_renderSample;
  self->renderTile    = Renderer_default_renderTile;
  self->beginFrame    = Renderer_default_beginFrame;
  self->endFrame      = Renderer_default_endFrame;
  Renderer_set(self, 1, 20, 0.001f, make_vec4f(0.f), NULL);
  precomputedHalton_create();
}

void Renderer_Constructor(uniform Renderer *uniform self,
                          void *uniform cppE,
                          const uniform int32 spp)
{
  Renderer_Constructor(self, cppE);
  self->spp = spp;
}

// Exports (called from C++) //////////////////////////////////////////////////

export void Renderer_renderTile(void *uniform _self,
                                void *uniform _fb,
                                void *uniform _camera,
                                void *uniform _world,
                                void *uniform perFrameData,
                                uniform Tile &tile,
                                uniform int jobID)
{
  Renderer *uniform self  = (Renderer * uniform) _self;
  FrameBuffer *uniform fb = (FrameBuffer * uniform) _fb;
  Camera *uniform camera  = (Camera * uniform) _camera;
  World *uniform world    = (World * uniform) _world;
  self->renderTile(self, fb, camera, world, perFrameData, tile, jobID);
}

export void *uniform Renderer_beginFrame(void *uniform _self,
                                         void *uniform _world)
{
  Renderer *uniform self = (Renderer * uniform) _self;
  World *uniform world   = (World * uniform) _world;
  return self->beginFrame(self, world);
}

export void Renderer_endFrame(void *uniform _self, void *uniform perFrameData)
{
  uniform Renderer *uniform self = (uniform Renderer * uniform) _self;
  self->endFrame(self, perFrameData);
}

export void Renderer_set(void *uniform _self,
                         const uniform int32 spp,
                         const uniform int32 maxDepth,
                         const uniform float minContribution,
                         const uniform vec4f &bgColor,
                         void *uniform _maxDepthTexture)
{
  uniform Renderer *uniform self = (uniform Renderer * uniform) _self;

  self->spp             = spp;
  self->maxDepth        = maxDepth;
  self->minContribution = minContribution;
  self->bgColor         = bgColor;
  self->maxDepthTexture = (uniform Texture2D * uniform) _maxDepthTexture;

  precomputeZOrder();
}

export void Renderer_pick(const void *uniform _self,
                          const void *uniform _fb,
                          const void *uniform _camera,
                          const void *uniform _world,
                          const uniform vec2f &screenPos,
                          uniform vec3f &pos,
                          uniform int32 &instID,
                          uniform int32 &primID,
                          uniform int32 &hit)
{
  const Renderer *uniform self  = (const Renderer *uniform)_self;
  const FrameBuffer *uniform fb = (const FrameBuffer *uniform)_fb;
  const Camera *uniform camera  = (const Camera *uniform)_camera;
  const World *uniform world    = (const World *uniform)_world;

  CameraSample cameraSample;
  cameraSample.screen.x = screenPos.x;
  cameraSample.screen.y = screenPos.y;
  // use center of lens and shutter time
  cameraSample.lens.x = 0.0f;
  cameraSample.lens.y = 0.0f;
  cameraSample.time   = 0.5f;

  Ray ray;
  camera->initRay(camera, ray, cameraSample);

  if (self->maxDepthTexture) {
    // always sample center of pixel
    const vec2f pixel = floor(screenPos * make_vec2f(fb->size.x, fb->size.y));
    const vec2f depthTexCoord = (pixel + 0.5f) * fb->rcpSize;
    DifferentialGeometry lookup;
    initDgFromTexCoord(lookup, depthTexCoord);
    const float tMax = get1f(self->maxDepthTexture, lookup);
    ray.t            = min(ray.t, tMax);
  }

  traceRay(world, ray);

  vec3f p = ray.org + ray.dir * ray.t;

  pos.x  = extract(p.x, 0);
  pos.y  = extract(p.y, 0);
  pos.z  = extract(p.z, 0);
  hit    = extract((int)(hadHit(ray)), 0);
  instID = extract(ray.instID, 0);
  primID = extract(ray.primID, 0);
}
