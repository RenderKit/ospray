// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// TODO: Seems like ISPC bug: if Renderer.ih included before
// Texture2D.ih I get an error that the Texture2D type is declared but not
// defined
#include "common/FeatureFlagsEnum.h"
#include "texture/Texture2D.ih"

#include "Renderer.ih"
#include "camera/Camera.ih"
#include "camera/CameraDispatch.ih"
#include "common/World.ih"
#include "fb/FrameBuffer.ih"
#include "fb/FrameBufferDispatch.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec4f Renderer_getBackground(const Renderer *uniform self,
    const vec2f &screenPos,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlags ff = getFeatureFlags(ffh);
  if (!self->backplate || !(ff.other & FFO_TEXTURE_IN_RENDERER))
    return self->bgColor;

  // TODO: Now for GPU making a whole DifferentialGeometry object
  // just to set up the texture coords we want to sample for the background
  // seems very wasteful
  DifferentialGeometry lookup;
  initDgFromTexCoord(lookup, clamp2edge(self->backplate, screenPos));
  return get4f(self->backplate, lookup);
}

SYCL_EXTERNAL float Renderer_getMaxDepth(const Renderer *uniform self,
    const vec2f &screenPos,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlags ff = getFeatureFlags(ffh);
  if (!self->maxDepthTexture || !(ff.other & FFO_TEXTURE_IN_RENDERER))
    return inf;

  DifferentialGeometry lookup;
  initDgFromTexCoord(lookup, screenPos);
  return max(0.f, get1f(self->maxDepthTexture, lookup));
}

// Exports (called from C++) //////////////////////////////////////////////////

export void Renderer_pick(const void *uniform _self,
    const void *uniform,
    const void *uniform _camera,
    const void *uniform _world,
    const uniform vec2f &screenPos,
    uniform vec3f &pos,
    uniform int32 &instID,
    uniform int32 &geomID,
    uniform int32 &primID,
    uniform int32 &hit)
{
#ifndef OSPRAY_TARGET_SYCL
  const Renderer *uniform self = (const Renderer *uniform)_self;
  const Camera *uniform camera = (const Camera *uniform)_camera;
  const World *uniform world = (const World *uniform)_world;

  CameraSample cameraSample;
  cameraSample.screen.x = screenPos.x;
  cameraSample.screen.y = screenPos.y;
  // use center of lens and shutter time
  cameraSample.lens.x = 0.0f;
  cameraSample.lens.y = 0.0f;
  cameraSample.time = 0.5f;

  uniform FeatureFlagsHandler ffh;
  Ray ray;
  Camera_dispatch_initRay(camera, ray, cameraSample, ffh);
  ray.t = min(ray.t, Renderer_getMaxDepth(self, cameraSample.screen, ffh));

  traceRay(world, ray, ffh);

  vec3f p = ray.org + ray.dir * ray.t;

  pos.x = extract(p.x, 0);
  pos.y = extract(p.y, 0);
  pos.z = extract(p.z, 0);
  hit = extract((int)(hadHit(ray)), 0);
  instID = extract(ray.instID, 0);
  geomID = extract(ray.geomID, 0);
  primID = extract(ray.primID, 0);
#else
  (void)_self;
  (void)_camera;
  (void)_world;
  (void)screenPos;
  (void)pos;
  (void)instID;
  (void)geomID;
  (void)primID;
  (void)hit;
#endif
}

OSPRAY_END_ISPC_NAMESPACE
