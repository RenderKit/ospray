// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Renderer common infrastructure shared among other renderers
static void Renderer_default_renderTask(const uniform vec3ui itemIndex,
    Renderer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    World *uniform world,
    const uint32 *uniform taskIDs,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform int32 spp = self->spp;

  ScreenSample screenSample;
  screenSample.z = inf;
  screenSample.alpha = 0.f;

  CameraSample cameraSample;

  uniform RenderTaskDesc taskDesc =
      FrameBuffer_dispatch_getRenderTaskDesc(fb, taskIDs[itemIndex.z], ffh);

  const uniform int startSampleID =
      fb->frameID * spp + 1; // Halton Sequence starts with 1

  if (fb->cancelRender || isEmpty(taskDesc.region)) {
    return;
  }

#ifndef ISPC
  {
    int32 y = taskDesc.region.lower.y + itemIndex.y;
    int32 x = taskDesc.region.lower.x + itemIndex.x;
#else
  foreach_tiled (y = taskDesc.region.lower.y... taskDesc.region.upper.y,
      x = taskDesc.region.lower.x... taskDesc.region.upper.x) {
#endif
    screenSample.sampleID.x = x;
    screenSample.sampleID.y = y;

    // set ray t value for early ray termination (from maximum depth texture)
    vec2f center =
        make_vec2f(screenSample.sampleID.x, screenSample.sampleID.y) + 0.5f;
    const float tMax = Renderer_getMaxDepth(self, center * fb->rcpSize, ffh);
    screenSample.z = tMax;

    vec3f col = make_vec3f(0.f);
    float alpha = 0.f;
    float depth = inf;
    vec3f normal = make_vec3f(0.f);
    vec3f albedo = make_vec3f(0.f);

    for (uniform int32 s = 0; s < spp; s++) {
      const float pixel_du = Halton_sample2(startSampleID + s);
      const float pixel_dv = CranleyPattersonRotation(
          Halton_sample3(startSampleID + s),
          1.f / 6.f); // rotate to sample center (0.5) of pixel for sampleID=0
      const vec2f pixelSample = make_vec2f(pixel_du, pixel_dv);

      const PixelFilter *uniform pf = self->pixelFilter;
      const vec2f pfSample = pf ? PixelFilter_dispatch_sample(pf, pixelSample)
                                : pixelSample - make_vec2f(0.5f);

      screenSample.sampleID.z = startSampleID + s;

      cameraSample.pixel_center = (make_vec2f(x, y) + 0.5f) * fb->rcpSize;
      screenSample.pos = cameraSample.pixel_center;
      cameraSample.screen = cameraSample.pixel_center + pfSample * fb->rcpSize;

      // no DoF or MB per default
      cameraSample.lens.x = 0.0f;
      cameraSample.lens.y = 0.0f;
      cameraSample.time = 0.5f;

      Camera_dispatch_initRay(
          camera, screenSample.ray, screenSample.rayCone, cameraSample, ffh);
      // take screen resolution (unnormalized coordinates), i.e., pixel size
      // into account
      screenSample.rayCone.dwdt *= fb->rcpSize.y;
      screenSample.rayCone.width *= fb->rcpSize.y;

      screenSample.ray.t = min(screenSample.ray.t, tMax);

      screenSample.z = inf;
      screenSample.primID = RTC_INVALID_GEOMETRY_ID;
      screenSample.geomID = RTC_INVALID_GEOMETRY_ID;
      screenSample.instID = RTC_INVALID_GEOMETRY_ID;
      screenSample.albedo =
          make_vec3f(Renderer_getBackground(self, screenSample.pos, ffh));
      screenSample.normal = make_vec3f(0.f);

      // The proper sample rendering function name is substituted here via macro
      renderSampleFn(self, fb, world, screenSample, ffh);

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

    FrameBuffer_dispatch_accumulateSample(fb, screenSample, taskDesc, ffh);
  }
  FrameBuffer_dispatch_completeTask(fb, taskDesc, ffh);
}
