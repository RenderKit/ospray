// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "camera/Camera.ih"
#include "common/Intersect.ih"
#include "common/Ray.ih"
#include "common/World.ih"
#include "fb/SparseFB.ih"
#include "math/random.ih"
#include "rkcommon/utility/random.ih"
#include "texture/Texture2D.ih"
// c++ shared
#include "DistributedRendererShared.h"
#include "common/DistributedWorldShared.h"
#include "fb/FrameBufferShared.h"
#include "fb/SparseFBShared.h"

unmasked void markRegionVisibleFilterFunction(
    const RTCFilterFunctionNArguments *uniform args)
{
  if (!args->valid[programIndex]) {
    return;
  }

  EmbreeIntersectionContext *uniform ctx =
      (EmbreeIntersectionContext * uniform) args->context;

  uint8 *uniform regionVisible = (uint8 * uniform) ctx->userPtr;
  varying RTCHit *uniform hit = (varying RTCHit * uniform) args->hit;
  regionVisible[hit->primID] = 0xff;

  // Reject all hits, we want to find all intersections along the ray to find
  // all visible regions
  args->valid[programIndex] = 0;
}

task void DR_default_computeRegionVisibility(DistributedRenderer *uniform self,
    SparseFB *uniform fb,
    Camera *uniform camera,
    DistributedWorld *uniform world,
    uint8 *uniform regionVisible,
    void *uniform perFrameData,
    const int *uniform taskIDs)
{
  // TODO: merge the copy-paste here somewhat?
  const uniform int32 spp = self->super.spp;

  ScreenSample screenSample;
  screenSample.z = inf;
  screenSample.alpha = 0.f;

  CameraSample cameraSample;

  uniform RenderTaskDesc taskDesc =
      fb->super.getRenderTaskDesc(&fb->super, taskIDs[taskIndex0]);

  if (fb->super.cancelRender || isEmpty(taskDesc.region)) {
    return;
  }

  // Get the region visibility info for this tile
  uint8 *uniform tileRegionVisible = regionVisible
      + world->numRegions * SparseFB_getTileIndexForTask(fb, taskDesc.taskID);

  // Take fixed, consistent samples to determine if a region is really visible
  // for a tile. We can trace up to 5 rays, one through the center of the pixel
  // and one through each corner, or 4 (one through each corner), or just 1
  // (through the middle). Tracing 5 against the boxes may be a bit much? It
  // shouldn't impact performance too badly, but is actually a lot of rays being
  // traced Alternatively we could just base this on the region projection, and
  // accept the more conservative bounds. This would require some benchmarking
  // to see where the scaling trade off hits of sending tiles we don't actually
  // need.
  // Right now just taking 1 sample, can benchmark this in the future and/or
  // refactor code further to use this result to do a scan/compaction on the
  // render task IDs to reduce tasks launched. Then the accumIDs can be sync'd
  // up again
  const uniform int nVisibilitySamples = 1;
  const uniform vec2f visibilitySamples[5] = {make_vec2f(0.5f, 0.5f),
      make_vec2f(0.0f, 0.0f),
      make_vec2f(1.0f, 0.0f),
      make_vec2f(0.0f, 1.0f),
      make_vec2f(1.0f, 1.0f)};

  foreach_tiled (y = taskDesc.region.lower.y... taskDesc.region.upper.y,
      x = taskDesc.region.lower.x... taskDesc.region.upper.x) {
    screenSample.sampleID.x = x;
    screenSample.sampleID.y = y;

    float tMax = inf;
    // set ray t value for early ray termination if we have a maximum depth
    // texture
    if (self->super.maxDepthTexture) {
      // always sample center of pixel
      vec2f depthTexCoord;
      depthTexCoord.x = (screenSample.sampleID.x + 0.5f) * fb->super.rcpSize.x;
      depthTexCoord.y = (screenSample.sampleID.y + 0.5f) * fb->super.rcpSize.y;

      DifferentialGeometry lookup;
      initDgFromTexCoord(lookup, depthTexCoord);
      tMax = min(get1f(self->super.maxDepthTexture, lookup), inf);
    }

    for (uniform uint32 s = 0; s < nVisibilitySamples; s++) {
      screenSample.sampleID.z = s;

      cameraSample.screen.x = (screenSample.sampleID.x + visibilitySamples[s].x)
          * fb->super.rcpSize.x;
      cameraSample.screen.y = (screenSample.sampleID.y + visibilitySamples[s].y)
          * fb->super.rcpSize.y;

      // no DoF or MB per default
      cameraSample.lens.x = 0.0f;
      cameraSample.lens.y = 0.0f;
      cameraSample.time = 0.5f;

      camera->initRay(camera, screenSample.ray, cameraSample);
      screenSample.ray.t = min(screenSample.ray.t, tMax);

      if (world->regionScene) {
        uniform EmbreeIntersectionContext context;
        rtcInitIntersectContext(&context.ectx);
        context.ectx.flags = RTC_INTERSECT_CONTEXT_FLAG_COHERENT;
        context.ectx.filter = markRegionVisibleFilterFunction;
        context.userPtr = tileRegionVisible;
        rtcIntersectV(world->regionScene,
            &context.ectx,
            (varying RTCRayHit * uniform) & screenSample.ray);
      }
    }
  }
}

void DR_default_renderRegionSample(DistributedRenderer *uniform self,
    SparseFB *uniform fb,
    DistributedWorld *uniform world,
    const box3f *uniform region,
    const vec2f &regionInterval,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  sample.z = inf;
  sample.alpha = 1.f;
  sample.rgb = make_random_color((sample.sampleID.x << 0)
      + (sample.sampleID.y << 14) + (sample.sampleID.z << 28));
}

task void DR_default_renderRegionToTile(DistributedRenderer *uniform self,
    SparseFB *uniform fb,
    Camera *uniform camera,
    DistributedWorld *uniform world,
    const box3f *uniform region,
    void *uniform perFrameData,
    const int *uniform taskIDs)
{
  const uniform int32 spp = self->super.spp;

  ScreenSample screenSample;
  screenSample.z = inf;
  screenSample.alpha = 0.f;

  CameraSample cameraSample;

  uniform RenderTaskDesc taskDesc =
      fb->super.getRenderTaskDesc(&fb->super, taskIDs[taskIndex0]);

  const uniform int startSampleID = max(taskDesc.accumID, 0) * spp;

  if (isEmpty(taskDesc.region)) {
    return;
  }

  foreach_tiled (y = taskDesc.region.lower.y... taskDesc.region.upper.y,
      x = taskDesc.region.lower.x... taskDesc.region.upper.x) {
    screenSample.sampleID.x = x;
    screenSample.sampleID.y = y;

    float tMax = inf;
    // set ray t value for early ray termination if we have a maximum depth
    // texture
    if (self->super.maxDepthTexture) {
      // always sample center of pixel
      vec2f depthTexCoord;
      depthTexCoord.x = (screenSample.sampleID.x + 0.5f) * fb->super.rcpSize.x;
      depthTexCoord.y = (screenSample.sampleID.y + 0.5f) * fb->super.rcpSize.y;

      DifferentialGeometry lookup;
      initDgFromTexCoord(lookup, depthTexCoord);
      tMax = min(get1f(self->super.maxDepthTexture, lookup), inf);
    }
    vec3f col = make_vec3f(0.f);
    float alpha = 0.f;
    vec3f normal = make_vec3f(0.f);
    vec3f albedo = make_vec3f(0.f);

    // TODO: same note on spp > 1 issues
    for (uniform uint32 s = 0; s < spp; s++) {
      const float pixel_du = Halton_sample2(startSampleID + s);
      const float pixel_dv = CranleyPattersonRotation(
          Halton_sample3(startSampleID + s), 1.f / 6.f);
      screenSample.sampleID.z = startSampleID + s;

      cameraSample.screen.x =
          (screenSample.sampleID.x + pixel_du) * fb->super.rcpSize.x;
      cameraSample.screen.y =
          (screenSample.sampleID.y + pixel_dv) * fb->super.rcpSize.y;

      // no DoF or MB per default
      cameraSample.lens.x = 0.0f;
      cameraSample.lens.y = 0.0f;
      cameraSample.time = 0.5f;

      camera->initRay(camera, screenSample.ray, cameraSample);
      screenSample.ray.t = min(screenSample.ray.t, tMax);

      // TODO: We could store and use the region t intervals from when
      // we did the visibility test?
      Intersections isect =
          intersectBox(screenSample.ray.org, screenSample.ray.dir, *region);

      if (isect.entry.t < isect.exit.t && isect.exit.t >= screenSample.ray.t0
          && isect.entry.t <= screenSample.ray.t) {
        const float regionEnter = max(isect.entry.t, screenSample.ray.t0);
        const float regionExit = min(isect.exit.t, screenSample.ray.t);
        screenSample.ray.t0 = regionEnter;
        screenSample.ray.t = regionExit;
        self->renderRegionSample(self,
            fb,
            world,
            region,
            make_vec2f(regionEnter, regionExit),
            perFrameData,
            screenSample);

        col = col + screenSample.rgb;
        alpha += screenSample.alpha;
        normal = normal + screenSample.normal;
        albedo = albedo + screenSample.albedo;
      }
    }
    const float rspp = rcpf(spp);
    screenSample.rgb = col * rspp;
    screenSample.alpha = alpha * rspp;
    screenSample.normal = normal * rspp;
    screenSample.albedo = albedo * rspp;

    fb->super.accumulateSample(&fb->super, screenSample, taskDesc);
  }
  fb->super.completeTask(&fb->super, taskDesc);
}

export void DistributedRenderer_computeRegionVisibility(void *uniform _self,
    void *uniform fb,
    void *uniform camera,
    void *uniform world,
    uint8 *uniform regionVisible,
    void *uniform perFrameData,
    void *uniform _taskIDs,
    uniform int numTasks)
{
  DistributedRenderer *uniform self =
      (uniform DistributedRenderer * uniform) _self;
  const int *uniform taskIDs = (const int *uniform)_taskIDs;
  launch[numTasks] self->computeRegionVisibility(self,
      (SparseFB * uniform) fb,
      (Camera * uniform) camera,
      (DistributedWorld * uniform) world,
      regionVisible,
      perFrameData,
      taskIDs);
}

export void DistributedRenderer_renderRegionToTile(void *uniform _self,
    void *uniform fb,
    void *uniform camera,
    void *uniform world,
    const void *uniform region,
    void *uniform perFrameData,
    void *uniform _taskIDs,
    uniform int numTasks)
{
  DistributedRenderer *uniform self =
      (uniform DistributedRenderer * uniform) _self;
  const int *uniform taskIDs = (const int *uniform)_taskIDs;
  launch[numTasks] self->renderRegionToTile(self,
      (SparseFB * uniform) fb,
      (Camera * uniform) camera,
      (DistributedWorld * uniform) world,
      (const box3f *uniform)region,
      perFrameData,
      taskIDs);
}

export void DistributedRenderer_pick(const void *uniform _self,
    const void *uniform _fb,
    const void *uniform _camera,
    const void *uniform _world,
    const uniform vec2f &screenPos,
    uniform vec3f &pos,
    uniform int32 &instID,
    uniform int32 &geomID,
    uniform int32 &primID,
    uniform float &depth,
    uniform int32 &hit)
{
  const Renderer *uniform self = (const Renderer *uniform)_self;
  const FrameBuffer *uniform fb = (const FrameBuffer *uniform)_fb;
  const Camera *uniform camera = (const Camera *uniform)_camera;
  const DistributedWorld *uniform world =
      (const DistributedWorld *uniform)_world;

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

  // Clip the ray to each region and trace the clipped ray to find the picked
  // object
  float closestHit = 1e20f;
  for (uniform int i = 0; i < world->numRegions; ++i) {
    // Separate ray per-region to allow clipping by the region's bounds
    Ray regionRay = ray;
    regionRay.t = min(ray.t, closestHit);

    const Intersections isect =
        intersectBox(regionRay.org, regionRay.dir, world->regions[i]);

    cif (isect.entry.t < isect.exit.t && isect.exit.t >= regionRay.t0
        && isect.entry.t <= regionRay.t) {
      regionRay.t0 = isect.entry.t;
      regionRay.t = min(regionRay.t, isect.exit.t);
      traceRay(&world->super, regionRay);

      cif (hadHit(regionRay)) {
        closestHit = regionRay.t;
        vec3f p = regionRay.org + regionRay.dir * regionRay.t;

        pos.x = extract(p.x, 0);
        pos.y = extract(p.y, 0);
        pos.z = extract(p.z, 0);
        hit = extract((int)(hadHit(regionRay)), 0);
        instID = extract(regionRay.instID, 0);
        geomID = extract(regionRay.geomID, 0);
        primID = extract(regionRay.primID, 0);
        depth = extract(regionRay.t, 0);
      }
    }
  }
}

export void *uniform DR_default_computeRegionVisibility_addr()
{
  return DR_default_computeRegionVisibility;
}

export void *uniform DR_default_renderRegionSample_addr()
{
  return DR_default_renderRegionSample;
}

export void *uniform DR_default_renderRegionToTile_addr()
{
  return DR_default_renderRegionToTile;
}
