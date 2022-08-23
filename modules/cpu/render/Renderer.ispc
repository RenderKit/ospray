// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// TODO: Seems like ISPC bug: if Renderer.ih included before
// Texture2D.ih I get an error that the Texture2D type is declared but not
// defined
#include "texture/Texture2D.ih"

#include "Renderer.ih"
#include "camera/Camera.ih"
#include "camera/CameraType.ih"
#include "common/World.ih"
#include "fb/FrameBufferType.ih"
#include "math/random.ih"
#include "pf/PixelFilterType.ih"
#include "render/util.ih"
// c++ shared
#include "fb/FrameBufferShared.h"

#ifdef OSPRAY_TARGET_DPCPP
#include "rkcommon/math/math.h"
#endif

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec4f Renderer_getBackground(
    const Renderer *uniform self, const vec2f &screenPos)
{
  if (!self->backplate)
    return self->bgColor;

  // TODO: Now for GPU making a whole DifferentialGeometry object
  // just to set up the texture coords we want to sample for the background
  // seems very wasteful
  DifferentialGeometry lookup;
  initDgFromTexCoord(lookup, clamp2edge(self->backplate, screenPos));
  return get4f(self->backplate, lookup);
}

SYCL_EXTERNAL float Renderer_getMaxDepth(
    const Renderer *uniform self, const vec2f &screenPos)
{
  if (!self->maxDepthTexture)
    return inf;

  DifferentialGeometry lookup;
  initDgFromTexCoord(lookup, screenPos);
  return max(0.f, get1f(self->maxDepthTexture, lookup));
}

SYCL_EXTERNAL void Renderer_default_renderSample(Renderer *uniform self,
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

#ifndef OSPRAY_TARGET_DPCPP
#include "RendererRenderTaskFn.inl"
#endif

// Exports (called from C++) //////////////////////////////////////////////////

#ifndef OSPRAY_TARGET_DPCPP
export void *uniform Renderer_default_renderSample_addr()
{
  return (void *uniform)Renderer_default_renderSample;
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
  sync;
}
#endif

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

OSPRAY_END_ISPC_NAMESPACE
