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

  uniform RenderTaskDesc taskDesc = FrameBuffer_dispatch_getRenderTaskDesc(
      &fb->super, taskIDs[taskIndex0], ffh);

  const uniform int startSampleID =
      fb->super.frameID * spp + 1; // Halton Sequence starts with 1

  if (isEmpty(taskDesc.region)) {
    return;
  }

#ifndef ISPC
  for (int32 y = taskDesc.region.lower.y; y < taskDesc.region.upper.y; ++y)
    for (int32 x = taskDesc.region.lower.x; x < taskDesc.region.upper.x; ++x) {
#else
  foreach_tiled (y = taskDesc.region.lower.y... taskDesc.region.upper.y,
      x = taskDesc.region.lower.x... taskDesc.region.upper.x) {
#endif
      ScreenSample screenSample = make_ScreenSample_zero();
      ScreenSample sample = make_ScreenSample_zero();
      CameraSample cameraSample;

      sample.sampleID.x = x;
      sample.sampleID.y = y;

      // set ray t value for early ray termination (from maximum depth texture)
      vec2f center =
          make_vec2f(screenSample.sampleID.x, screenSample.sampleID.y) + 0.5f;
      const float tMax =
          Renderer_getMaxDepth(self, center * fb->super.rcpSize, ffh);

      for (uniform int32 s = 0; s < spp; s++) {
        const float pixel_du = Halton_sample2(startSampleID + s);
        const float pixel_dv = CranleyPattersonRotation(
            Halton_sample3(startSampleID + s),
            1.f / 6.f); // rotate to sample center (0.5) of pixel for sampleID=0
        const vec2f pfSample =
            make_vec2f(pixel_du, pixel_dv) - make_vec2f(0.5f);

        sample.sampleID.z = startSampleID + s;

        cameraSample.pixel_center =
            (make_vec2f(x, y) + 0.5f) * fb->super.rcpSize;
        sample.pos = cameraSample.pixel_center;
        cameraSample.screen =
            cameraSample.pixel_center + pfSample * fb->super.rcpSize;

        // no DoF or MB per default
        cameraSample.lens.x = 0.0f;
        cameraSample.lens.y = 0.0f;
        cameraSample.time = 0.5f;

        Camera_dispatch_initRay(
            camera, sample.ray, sample.rayCone, cameraSample, ffh);
        // take screen resolution (unnormalized coordinates), i.e., pixel size
        // into account
        sample.rayCone.dwdt *= fb->super.rcpSize.y;
        sample.rayCone.width *= fb->super.rcpSize.y;

        sample.ray.t = min(sample.ray.t, tMax);

        // TODO: We could store and use the region t intervals from when
        // we did the visibility test?
        Intersections isect =
            intersectBox(sample.ray.org, sample.ray.dir, *region);

        if (isect.entry.t < isect.exit.t && isect.exit.t >= sample.ray.t0
            && isect.entry.t <= sample.ray.t) {
          const float regionEnter = max(isect.entry.t, sample.ray.t0);
          const float regionExit = min(isect.exit.t, sample.ray.t);
          sample.ray.t0 = regionEnter;
          sample.ray.t = regionExit;
          renderRegionSampleFn(self,
              fb,
              world,
              region,
              make_vec2f(regionEnter, regionExit),
              perFrameData,
              sample,
              ffh);

          ScreenSample_accumulate(screenSample, sample);
        }
      }

      ScreenSample_normalize(screenSample, spp);
      screenSample.sampleID.x = x;
      screenSample.sampleID.y = y;

      FrameBuffer_dispatch_accumulateSample(
          &fb->super, screenSample, taskDesc, ffh);
    }
  FrameBuffer_dispatch_completeTask(&fb->super, taskDesc, ffh);
}
