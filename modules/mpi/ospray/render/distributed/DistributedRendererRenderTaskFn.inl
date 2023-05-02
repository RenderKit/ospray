// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef OSPRAY_TARGET_SYCL
task
#endif
    static void
    DR_default_renderRegionToTile(Renderer *uniform self,
        SparseFB *uniform fb,
        Camera *uniform camera,
        DistributedWorld *uniform world,
        const box3f *uniform region,
        void *uniform perFrameData,
        const uint32 *uniform taskIDs,
#ifdef OSPRAY_TARGET_SYCL
        const int taskIndex0,
#endif
        const uniform FeatureFlagsHandler &ffh)
{
  const uniform int32 spp = self->spp;

  ScreenSample screenSample;
  screenSample.z = inf;
  screenSample.alpha = 0.f;

  CameraSample cameraSample;

  uniform RenderTaskDesc taskDesc = FrameBuffer_dispatch_getRenderTaskDesc(
      &fb->super, taskIDs[taskIndex0], ffh);

  const uniform int startSampleID = max(taskDesc.accumID, 0) * spp;

#ifdef OSPRAY_TARGET_SYCL
#if 0
  // Dummy top level print so that prints at lower levels of the kernel
  // will work See JIRA https://jira.devtools.intel.com/browse/XDEPS-4729
  if (taskIndex0 == 0) {
      cl::sycl::ext::oneapi::experimental::printf("");
  }
#endif
#endif

  if (isEmpty(taskDesc.region)) {
    return;
  }

#ifdef OSPRAY_TARGET_SYCL
  for (int32 y = taskDesc.region.lower.y; y < taskDesc.region.upper.y; ++y)
    for (int32 x = taskDesc.region.lower.x; x < taskDesc.region.upper.x; ++x) {
#else
  foreach_tiled (y = taskDesc.region.lower.y... taskDesc.region.upper.y,
      x = taskDesc.region.lower.x... taskDesc.region.upper.x) {
#endif
      screenSample.sampleID.x = x;
      screenSample.sampleID.y = y;

      // set ray t value for early ray termination (from maximum depth texture)
      vec2f center =
          make_vec2f(screenSample.sampleID.x, screenSample.sampleID.y) + 0.5f;
      const float tMax =
          Renderer_getMaxDepth(self, center * fb->super.rcpSize, ffh);
      vec3f col = make_vec3f(0.f);
      float alpha = 0.f;
      vec3f normal = make_vec3f(0.f);
      vec3f albedo = make_vec3f(0.f);

      // TODO: same note on spp > 1 issues
      for (uniform uint32 s = 0; s < spp; s++) {
        const float pixel_du = Halton_sample2(startSampleID + s);
        const float pixel_dv = CranleyPattersonRotation(
            Halton_sample3(self->mathConstants, startSampleID + s), 1.f / 6.f);
        screenSample.sampleID.z = startSampleID + s;

        cameraSample.screen.x =
            (screenSample.sampleID.x + pixel_du) * fb->super.rcpSize.x;
        cameraSample.screen.y =
            (screenSample.sampleID.y + pixel_dv) * fb->super.rcpSize.y;

        // no DoF or MB per default
        cameraSample.lens.x = 0.0f;
        cameraSample.lens.y = 0.0f;
        cameraSample.time = 0.5f;

        Camera_dispatch_initRay(camera, screenSample.ray, cameraSample, ffh);
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
          renderRegionSampleFn(self,
              fb,
              world,
              region,
              make_vec2f(regionEnter, regionExit),
              perFrameData,
              screenSample,
              ffh);

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

      FrameBuffer_dispatch_accumulateSample(
          &fb->super, screenSample, taskDesc, ffh);
    }
  FrameBuffer_dispatch_completeTask(&fb->super, taskDesc, ffh);
}
