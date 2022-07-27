// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.ih"
#include "camera/Camera.ih"
#include "common/World.ih"
#include "math/random.ih"
#include "pf/PixelFilter.ih"
#include "render/util.ih"
#include "texture/Texture2D.ih"
// c++ shared
#include "fb/FrameBufferShared.h"

vec4f Renderer_getBackground(
    const Renderer *uniform self, const vec2f &screenPos)
{
  if (!self->backplate)
    return self->bgColor;

  DifferentialGeometry lookup;
  initDgFromTexCoord(lookup, clamp2edge(self->backplate, screenPos));

  return get4f(self->backplate, lookup);
}

float Renderer_getMaxDepth(const Renderer *uniform self, const vec2f &screenPos)
{
  if (!self->maxDepthTexture)
    return inf;

  DifferentialGeometry lookup;
  initDgFromTexCoord(lookup, screenPos);
  return max(0.f, get1f(self->maxDepthTexture, lookup));
}

void Renderer_default_renderSample(Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform model,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  sample.z = sample.ray.t;
  vec4f bg = Renderer_getBackground(self, sample.pos);
  sample.alpha = bg.w;
  sample.rgb = make_vec3f(bg);
}

task void Renderer_default_renderTask(Renderer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    World *uniform world,
    void *uniform perFrameData,
    const uint32 *uniform taskIDs)
{
  const uniform int32 spp = self->spp;

  ScreenSample screenSample;
  screenSample.z = inf;
  screenSample.alpha = 0.f;

  CameraSample cameraSample;

  uniform RenderTaskDesc taskDesc =
      fb->getRenderTaskDesc(fb, taskIDs[taskIndex0]);

  const uniform int startSampleID = max(taskDesc.accumID, 0) * spp;

  if (fb->cancelRender || isEmpty(taskDesc.region)) {
    return;
  }

  foreach_tiled (y = taskDesc.region.lower.y... taskDesc.region.upper.y,
      x = taskDesc.region.lower.x... taskDesc.region.upper.x) {
    screenSample.sampleID.x = x;
    screenSample.sampleID.y = y;

    // set ray t value for early ray termination (from maximum depth texture)
    vec2f center =
        make_vec2f(screenSample.sampleID.x, screenSample.sampleID.y) + 0.5f;
    const float tMax = Renderer_getMaxDepth(self, center * fb->rcpSize);

    vec3f col = make_vec3f(0.f);
    float alpha = 0.f;
    float depth = inf;
    vec3f normal = make_vec3f(0.f);
    vec3f albedo = make_vec3f(0.f);

    for (uniform uint32 s = 0; s < spp; s++) {
      const float pixel_du = Halton_sample2(startSampleID + s);
      const float pixel_dv = CranleyPattersonRotation(
          Halton_sample3(startSampleID + s),
          1.f / 6.f); // rotate to sample center (0.5) of pixel for sampleID=0
      const vec2f pixelSample = make_vec2f(pixel_du, pixel_dv);

      const PixelFilter *uniform pf = self->pixelFilter;
      vec2f pfSample = pixelSample;
      if (pf) {
        pfSample = pf->sample(pf, pixelSample) + make_vec2f(0.5f);
      }

      screenSample.sampleID.z = startSampleID + s;

      cameraSample.screen.x =
          (screenSample.sampleID.x + pfSample.x) * fb->rcpSize.x;
      cameraSample.screen.y =
          (screenSample.sampleID.y + pfSample.y) * fb->rcpSize.y;
      screenSample.pos = cameraSample.screen;

      // no DoF or MB per default
      cameraSample.lens.x = 0.0f;
      cameraSample.lens.y = 0.0f;
      cameraSample.time = 0.5f;

      camera->initRay(camera, screenSample.ray, cameraSample);
      screenSample.ray.t = min(screenSample.ray.t, tMax);

      screenSample.z = inf;
      screenSample.primID = RTC_INVALID_GEOMETRY_ID;
      screenSample.geomID = RTC_INVALID_GEOMETRY_ID;
      screenSample.instID = RTC_INVALID_GEOMETRY_ID;
      screenSample.albedo =
          make_vec3f(Renderer_getBackground(self, screenSample.pos));
      screenSample.normal = make_vec3f(0.f);

      self->renderSample(self, fb, world, perFrameData, screenSample);
      col = col + screenSample.rgb;
      alpha += screenSample.alpha;
      depth = min(depth, screenSample.z);
      normal = normal + screenSample.normal;
      albedo = albedo + screenSample.albedo;
    }

    const float rspp = rcpf(spp);
    screenSample.rgb = col * rspp;
    screenSample.alpha = alpha * rspp;
    screenSample.z = depth;
    screenSample.normal = normal * rspp;
    screenSample.albedo = albedo * rspp;

    // Accumulate rendering result directly into the framebuffer, to remove
    // the need to call setTile separately on the CPU (but image ops needs
    // handling separately, etc.)
    fb->accumulateSample(fb, screenSample, taskDesc);
  }
  fb->completeTask(fb, taskDesc);
}

// Exports (called from C++) //////////////////////////////////////////////////

export void *uniform Renderer_default_renderSample_addr()
{
  return Renderer_default_renderSample;
}

export void Renderer_renderTasks(void *uniform _self,
    void *uniform _fb,
    void *uniform _camera,
    void *uniform _world,
    void *uniform perFrameData,
    void *uniform _taskIDs,
    uniform uint32 numTasks)
{
  Renderer *uniform self = (Renderer * uniform) _self;
  FrameBuffer *uniform fb = (FrameBuffer * uniform) _fb;
  Camera *uniform camera = (Camera * uniform) _camera;
  World *uniform world = (World * uniform) _world;
  const uint32 *uniform taskIDs = (const uint32 *uniform)_taskIDs;
  launch[numTasks] Renderer_default_renderTask(
      self, fb, camera, world, perFrameData, taskIDs);
}

export void Renderer_pick(const void *uniform _self,
    const void *uniform _fb,
    const void *uniform _camera,
    const void *uniform _world,
    const uniform vec2f &screenPos,
    uniform vec3f &pos,
    uniform int32 &instID,
    uniform int32 &geomID,
    uniform int32 &primID,
    uniform int32 &hit)
{
  const Renderer *uniform self = (const Renderer *uniform)_self;
  const FrameBuffer *uniform fb = (const FrameBuffer *uniform)_fb;
  const Camera *uniform camera = (const Camera *uniform)_camera;
  const World *uniform world = (const World *uniform)_world;

  CameraSample cameraSample;
  cameraSample.screen.x = screenPos.x;
  cameraSample.screen.y = screenPos.y;
  // use center of lens and shutter time
  cameraSample.lens.x = 0.0f;
  cameraSample.lens.y = 0.0f;
  cameraSample.time = 0.5f;

  Ray ray;
  camera->initRay(camera, ray, cameraSample);
  ray.t = min(ray.t, Renderer_getMaxDepth(self, cameraSample.screen));

  traceRay(world, ray);

  vec3f p = ray.org + ray.dir * ray.t;

  pos.x = extract(p.x, 0);
  pos.y = extract(p.y, 0);
  pos.z = extract(p.z, 0);
  hit = extract((int)(hadHit(ray)), 0);
  instID = extract(ray.instID, 0);
  geomID = extract(ray.geomID, 0);
  primID = extract(ray.primID, 0);
}
