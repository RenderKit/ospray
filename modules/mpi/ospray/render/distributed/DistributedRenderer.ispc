// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OSPConfig.h"
#include "fb/RenderTaskDesc.ih"
#include "render/ScreenSample.ih"
#include "rkcommon/math/vec.ih"
#include "rkcommon/utility/random.ih"

#include "camera/Camera.ih"
#include "camera/CameraDispatch.ih"
#include "common/FilterIntersect.ih"
#include "common/Intersect.ih"
#include "common/Ray.ih"
#include "common/RayQueryContext.ih"
#include "common/World.ih"
#include "fb/RenderTaskDesc.ih"
#include "fb/SparseFB.ih"
#include "geometry/BoxesShared.h"
#include "geometry/GeometryDispatch.ih"
#include "math/random.ih"
#include "render/ScreenSample.ih"
#include "rkcommon/utility/random.ih"
#include "texture/Texture2D.ih"
// c++ shared
#include "common/DistributedWorldShared.h"
#include "fb/FrameBufferDispatch.ih"
#include "fb/FrameBufferShared.h"
#include "fb/SparseFBShared.h"
#include "render/RendererShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

struct RayQueryContextRegion
{
  RTCRayQueryContext ectx;
  const FeatureFlagsHandler *uniform ffh;
  uint8 *regionVisible;
};

RTC_SYCL_INDIRECTLY_CALLABLE unmasked void Regions_intersect_kernel(
    const RTCIntersectFunctionNArguments *uniform args)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) {
    return;
  }
  // We never mark hits because we want to find all hits along the ray to get
  // all visible regions
  args->valid[programIndex] = 0;

  Boxes *uniform self = (Boxes * uniform) args->geometryUserPtr;
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  uniform int primID = args->primID;

  uniform box3f box = get_box3f(self->boxes, primID);

  // Note: not calling filterIntersectionBoth here because it will set the hit
  // on the ray, we don't want to record any hits so that we can find all
  // regions along the ray
  const Intersections isect = intersectBox(ray->org, ray->dir, box);
  if ((isect.entry.hit && isect.entry.t > ray->t0 && isect.entry.t <= ray->t)
      || (isect.exit.hit && isect.exit.t > ray->t0 && isect.exit.t <= ray->t)) {
    RayQueryContextRegion *uniform ctx =
        (RayQueryContextRegion * uniform) args->context;

    uint8 *uniform regionVisible = (uint8 * uniform) ctx->regionVisible;
    regionVisible[primID] = 0xff;
  }
}

#ifndef OSPRAY_TARGET_SYCL
task
#endif
    SYCL_EXTERNAL void
    DR_default_computeRegionVisibility(Renderer *uniform self,
        SparseFB *uniform fb,
        Camera *uniform camera,
        DistributedWorld *uniform world,
        uint8 *uniform regionVisible,
        void *uniform perFrameData,
        const uint32 *uniform taskIDs,
#ifdef OSPRAY_TARGET_SYCL
        const int taskIndex0,
#endif
        const uniform FeatureFlagsHandler &ffh)
{
  ScreenSample screenSample;
  screenSample.z = inf;
  screenSample.alpha = 0.f;

  CameraSample cameraSample;

  uniform RenderTaskDesc taskDesc = FrameBuffer_dispatch_getRenderTaskDesc(
      &fb->super, taskIDs[taskIndex0], ffh);

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

#ifdef OSPRAY_TARGET_SYCL
  for (int32 y = taskDesc.region.lower.y; y < taskDesc.region.upper.y; ++y)
    for (int32 x = taskDesc.region.lower.x; x < taskDesc.region.upper.x; ++x) {
#else
  foreach_tiled (y = taskDesc.region.lower.y... taskDesc.region.upper.y,
      x = taskDesc.region.lower.x... taskDesc.region.upper.x) {
#endif
      screenSample.sampleID.x = x;
      screenSample.sampleID.y = y;

      // set ray t value for early ray termination if we have a maximum depth
      // texture
      vec2f center =
          make_vec2f(screenSample.sampleID.x, screenSample.sampleID.y) + 0.5f;
      const float tMax = inf;
      // Renderer_getMaxDepth(&self->super, center * fb->super.rcpSize);

      for (uniform uint32 s = 0; s < nVisibilitySamples; s++) {
        screenSample.sampleID.z = s;

        cameraSample.screen.x =
            (screenSample.sampleID.x + visibilitySamples[s].x)
            * fb->super.rcpSize.x;
        cameraSample.screen.y =
            (screenSample.sampleID.y + visibilitySamples[s].y)
            * fb->super.rcpSize.y;

        // no DoF or MB per default
        cameraSample.lens.x = 0.0f;
        cameraSample.lens.y = 0.0f;
        cameraSample.time = 0.5f;

        Camera_dispatch_initRay(camera, screenSample.ray, cameraSample, ffh);
        screenSample.ray.t = min(screenSample.ray.t, tMax);

        if (world->regionScene) {
          uniform RayQueryContextRegion context;
          rtcInitRayQueryContext(&context.ectx);
          context.ffh = &ffh;
          context.regionVisible = tileRegionVisible;

          uniform RTCIntersectArguments intersectArgs;
          rtcInitIntersectArguments(&intersectArgs);
          // Use a custom intersection function instead of a global filter
          // function, since global filter functions were removed in Embree4
          intersectArgs.intersect =
              (RTCIntersectFunctionN)Regions_intersect_kernel;
          intersectArgs.flags = RTC_RAY_QUERY_FLAG_COHERENT;
          intersectArgs.feature_mask = (uniform RTCFeatureFlags)(
              RTC_FEATURE_FLAG_USER_GEOMETRY_CALLBACK_IN_ARGUMENTS);
          intersectArgs.context = &context.ectx;

          rtcIntersectV(world->regionScene,
              (varying RTCRayHit * uniform) & screenSample.ray,
              &intersectArgs);
        }
      }
    }
}

#ifndef OSPRAY_TARGET_SYCL
export void DistributedRenderer_computeRegionVisibility(void *uniform _self,
    void *uniform fb,
    void *uniform camera,
    void *uniform world,
    uint8 *uniform regionVisible,
    void *uniform perFrameData,
    void *uniform _taskIDs,
    uniform int numTasks)
{
  Renderer *uniform self = (uniform Renderer * uniform) _self;
  const uint32 *uniform taskIDs = (const uint32 *uniform)_taskIDs;
  uniform FeatureFlagsHandler ffh;
  launch[numTasks] DR_default_computeRegionVisibility(self,
      (SparseFB * uniform) fb,
      (Camera * uniform) camera,
      (DistributedWorld * uniform) world,
      regionVisible,
      perFrameData,
      taskIDs,
      ffh);
}

#ifndef OSPRAY_TARGET_SYCL
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
  uniform FeatureFlagsHandler ffh;
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
  Camera_dispatch_initRay(camera, ray, cameraSample, ffh);
  ray.t = min(ray.t, Renderer_getMaxDepth(self, cameraSample.screen, ffh));

  // Clip the ray to each region this rank owns and trace the clipped ray to
  // find the picked object
  float closestHit = 1e20f;
  for (uniform int i = 0; i < world->numLocalRegions; ++i) {
    // Separate ray per-region to allow clipping by the region's bounds
    Ray regionRay = ray;
    regionRay.t = min(ray.t, closestHit);

    const Intersections isect =
        intersectBox(regionRay.org, regionRay.dir, world->localRegions[i]);

    if (isect.entry.t < isect.exit.t && isect.exit.t >= regionRay.t0
        && isect.entry.t <= regionRay.t) {
      regionRay.t0 = isect.entry.t;
      regionRay.t = min(regionRay.t, isect.exit.t);
      traceRay(&world->super, regionRay, ffh);

      if (hadHit(regionRay)) {
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
#endif

#endif

OSPRAY_END_ISPC_NAMESPACE
