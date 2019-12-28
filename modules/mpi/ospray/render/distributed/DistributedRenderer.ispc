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

#include "DistributedRenderer.ih"
#include "common/DistributedWorld.ih"
#include "ospray/camera/Camera.ih"
#include "ospray/fb/FrameBuffer.ih"
#include "ospray/fb/Tile.ih"
#include "ospray/render/Renderer.ih"
#include "ospray/render/util.ih"
#include "ospray/common/Intersect.ih"

unmasked void DR_default_computeRegionVisibility(
    DistributedRenderer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    DistributedWorld *uniform world,
    bool *uniform regionVisible,
    void *uniform perFrameData,
    uniform Tile &tile,
    uniform int taskIndex)
{
  // TODO: merge the copy-paste here somewhat?
  const uniform int32 spp = self->super.spp;

  ScreenSample screenSample;
  screenSample.z = inf;
  screenSample.alpha = 0.f;

  CameraSample cameraSample;

  const uniform int begin = taskIndex * RENDERTILE_PIXELS_PER_JOB;
  const uniform int end = begin + RENDERTILE_PIXELS_PER_JOB;
  const uniform int startSampleID = max(tile.accumID, 0) * spp;

  for (uniform uint32 i = begin; i < end; i += programCount) {
    const uint32 index = i + programIndex;
    screenSample.sampleID.x = tile.region.lower.x + z_order.xs[index];
    screenSample.sampleID.y = tile.region.lower.y + z_order.ys[index];

    if ((screenSample.sampleID.x >= fb->size.x)
        || (screenSample.sampleID.y >= fb->size.y))
      continue;

    float tMax = inf;
    // set ray t value for early ray termination if we have a maximum depth
    // texture
    if (self->super.maxDepthTexture) {
      // always sample center of pixel
      vec2f depthTexCoord;
      depthTexCoord.x = (screenSample.sampleID.x + 0.5f) * fb->rcpSize.x;
      depthTexCoord.y = (screenSample.sampleID.y + 0.5f) * fb->rcpSize.y;

      DifferentialGeometry lookup;
      initDgFromTexCoord(lookup, depthTexCoord);
      tMax = min(get1f(self->super.maxDepthTexture, lookup), inf);
    }

    const uint32 pixel = z_order.xs[index] + (z_order.ys[index] * TILE_SIZE);

    // TODO: Does spp still show some block boundary artifacts? Check
    // and maybe just support spp = 1 in DR
    for (uniform uint32 s = 0; s < spp; s++) {
      const float pixel_du = precomputedHalton2(startSampleID + s);
      const float pixel_dv = precomputedHalton3(startSampleID + s);
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

      for (uniform int r = 0; r < world->numRegions; ++r) {
        if (!regionVisible[r] && !isEmpty(world->regions[r].bounds)) {
          Intersections isect = intersectBox(screenSample.ray.org,
                                             screenSample.ray.dir,
                                             world->regions[r].bounds);
          if (isect.entry.t < isect.exit.t
              && isect.exit.t >= screenSample.ray.t0
              && isect.entry.t <= screenSample.ray.t)
          {
            regionVisible[r] = true;
          }
        }
      }
    }

    screenSample.rgb = make_vec3f(
        self->super.bgColor.x, self->super.bgColor.y, self->super.bgColor.z);
    screenSample.alpha = self->super.bgColor.z;
    screenSample.normal = make_vec3f(0.f);
    screenSample.albedo = make_vec3f(0.f);
    setTile(tile, pixel, screenSample);
  }
}

void DR_default_renderRegionSample(DistributedRenderer *uniform self,
    FrameBuffer *uniform fb,
    DistributedWorld *uniform world,
    const Region *uniform region,
    const vec2f &regionInterval,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  sample.z = inf;
  sample.alpha = 1.f;
  sample.rgb = make_random_color((sample.sampleID.x << 0)
      + (sample.sampleID.y << 14) + (sample.sampleID.z << 28));
}

unmasked void DR_default_renderRegionToTile(DistributedRenderer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    DistributedWorld *uniform world,
    const Region *uniform region,
    void *uniform perFrameData,
    uniform Tile &tile,
    uniform int taskIndex)
{
  const uniform int32 spp = self->super.spp;

  ScreenSample screenSample;
  screenSample.z = inf;
  screenSample.alpha = 0.f;

  CameraSample cameraSample;

  const uniform int begin = taskIndex * RENDERTILE_PIXELS_PER_JOB;
  const uniform int end = begin + RENDERTILE_PIXELS_PER_JOB;
  const uniform int startSampleID = max(tile.accumID, 0) * spp;

  for (uniform uint32 i = begin; i < end; i += programCount) {
    const uint32 index = i + programIndex;
    screenSample.sampleID.x = tile.region.lower.x + z_order.xs[index];
    screenSample.sampleID.y = tile.region.lower.y + z_order.ys[index];

    if ((screenSample.sampleID.x >= fb->size.x)
        || (screenSample.sampleID.y >= fb->size.y))
      continue;

    float tMax = inf;
    // set ray t value for early ray termination if we have a maximum depth
    // texture
    if (self->super.maxDepthTexture) {
      // always sample center of pixel
      vec2f depthTexCoord;
      depthTexCoord.x = (screenSample.sampleID.x + 0.5f) * fb->rcpSize.x;
      depthTexCoord.y = (screenSample.sampleID.y + 0.5f) * fb->rcpSize.y;

      DifferentialGeometry lookup;
      initDgFromTexCoord(lookup, depthTexCoord);
      tMax = min(get1f(self->super.maxDepthTexture, lookup), inf);
    }
    vec3f col = make_vec3f(0.f);
    float alpha = 0.f;
    vec3f normal = make_vec3f(0.f);
    vec3f albedo = make_vec3f(0.f);

    const uint32 pixel = z_order.xs[index] + (z_order.ys[index] * TILE_SIZE);

    // TODO: same note on spp > 1 issues
    for (uniform uint32 s = 0; s < spp; s++) {
      const float pixel_du = precomputedHalton2(startSampleID + s);
      const float pixel_dv = precomputedHalton3(startSampleID + s);
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

      // TODO: We could store and use the region t intervals from when
      // we did the visibility test?
      Intersections isect = intersectBox(screenSample.ray.org,
                                         screenSample.ray.dir,
                                         region->bounds);

      if (isect.entry.t < isect.exit.t
          && isect.exit.t >= screenSample.ray.t0
          && isect.entry.t <= screenSample.ray.t)
      {
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
    setTile(tile, pixel, screenSample);
  }
}

void DistributedRenderer_Constructor(
    uniform DistributedRenderer *uniform self, void *uniform cppE)
{
  Renderer_Constructor(&self->super, cppE, 1);

  self->computeRegionVisibility = DR_default_computeRegionVisibility;
  self->renderRegionSample = DR_default_renderRegionSample;
  self->renderRegionToTile = DR_default_renderRegionToTile;
}

export void DistributedRenderer_computeRegionVisibility(void *uniform _self,
    void *uniform fb,
    void *uniform camera,
    void *uniform world,
    bool *uniform regionVisible,
    void *uniform perFrameData,
    uniform Tile &tile,
    uniform int taskIndex)
{
  DistributedRenderer *uniform self =
      (uniform DistributedRenderer * uniform) _self;
  self->computeRegionVisibility(self,
      (FrameBuffer * uniform) fb,
      (Camera * uniform) camera,
      (DistributedWorld * uniform) world,
      regionVisible,
      perFrameData,
      tile,
      taskIndex);
}

export void DistributedRenderer_renderRegionToTile(void *uniform _self,
    void *uniform fb,
    void *uniform camera,
    void *uniform world,
    const void *uniform region,
    void *uniform perFrameData,
    uniform Tile &tile,
    uniform int taskIndex)
{
  DistributedRenderer *uniform self =
      (uniform DistributedRenderer * uniform) _self;
  self->renderRegionToTile(self,
      (FrameBuffer * uniform) fb,
      (Camera * uniform) camera,
      (DistributedWorld * uniform) world,
      (const Region *uniform)region,
      perFrameData,
      tile,
      taskIndex);
}
