// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "ospray/fb/FrameBuffer.ih"
#include "ospray/common/Model.ih"
#include "ospray/common/Ray.ih"
#include "ospray/render/util.ih"
#include "ospray/render/volume/RaycastVolumeRenderer.ih"
#include "ospray/render/volume/RaycastVolumeMaterial.ih"
#include "ospray/volume/DataDistributedBlockedVolume.ih"
#include "ospray/render/util.ih"
#include "ospray/camera/Camera.ih"
#include "ospray/common/Model.ih"

struct PassInfo {
  // region to integrate over in this pass
  varying region1f region;
  // block to use in this pass - NULL for 'everything other than blocks'
  DDBVolumeBlock *uniform block;
  // blend in background color when ray misses
  uniform bool useBG;
};

inline void RaycastVolumeRenderer_computeVolumeSample(RaycastVolumeRenderer *uniform self,
                                                      Volume *uniform volume,
                                                      varying Ray &ray,
                                                      varying vec4f &color)
{
  // Sample the volume at the hit point in world coordinates.
  const vec3f coordinates = ray.org + ray.t0 * ray.dir;
  const float sample = volume->computeSample(volume, coordinates);

  TransferFunction *uniform xf = volume->transferFunction;

  // Look up the color associated with the volume sample.
  vec3f sampleColor = xf->getColorForValue(xf, sample);

  // Compute gradient shading, if enabled.
  if(volume->gradientShadingEnabled) {

    // Use volume gradient as the normal.
    const vec3f gradient = safe_normalize(volume->computeGradient(volume, coordinates));

    // Setup differential geometry for the volume sample point.
    DifferentialGeometry dg;
    dg.P = coordinates;
    dg.Ns = gradient;

    // Compute lighting.
    vec3f shadedColor = make_vec3f(0.f);
    const vec2f s = make_vec2f(0.5f);

    for (uniform uint32 i=0; i<self->numLights; i++) {
      const Light_SampleRes light = self->lights[i]->sample(self->lights[i], dg, s);
      const float cosNL = (gradient.x == 0.f && gradient.y == 0.f && gradient.z == 0.f) ?
                          1.f : abs(dot(safe_normalize(light.dir), gradient));

      shadedColor = shadedColor + sampleColor * cosNL * light.weight;
    }

    sampleColor = shadedColor;
  }

  // Look up the opacity associated with the volume sample.
  const float sampleOpacity = xf->getOpacityForValue(xf, sample);

  // Set the color contribution for this sample only (do not accumulate).
  color
    = clamp(sampleOpacity / volume->samplingRate) 
    * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);

  // Advance the ray for the next sample.
  volume->intersect(volume, ray);
}

inline void RaycastVolumeRenderer_computeGeometrySample(RaycastVolumeRenderer *uniform self,
                                                        varying Ray &ray,
                                                        const region1f &clipRange,
                                                        varying vec4f &color)
{
  // We compute intersections on the model and provide the contribution for the closest hit.
  traceRay(self->super.model, ray);

  // No hit found.
  if(ray.geomID < 0) {
    ray.t = infinity;
    return;
  }

  // Post intersect on the hit geometry.
  DifferentialGeometry dg;
  postIntersect(self->super.model, dg, ray,
                DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_COLOR
                |DG_MATERIALID|DG_TEXCOORD);

  // Color of the geometry.
  vec3f geometryColor = make_vec3f(dg.color.x, dg.color.y, dg.color.z);

  // Apply material if it exists.
  uniform RaycastVolumeRendererMaterial *mat
    = (uniform RaycastVolumeRendererMaterial*)dg.material;

  float d  = 1.f;
  vec3f Kd = make_vec3f(0.f,1.f,0.f);

  if (mat) {
    foreach_unique(m in mat) {
      d = m->d * get1f(m->map_d, dg.st, 1.f);
      Kd = m->Kd * make_vec3f(dg.color);
      if (m->map_Kd) {
        vec4f Kd_from_map = get4f(m->map_Kd,dg.st);
        Kd = Kd * make_vec3f(Kd_from_map);
        d *= Kd_from_map.w;
      } else {
        Kd = m->Kd;
      }

      geometryColor = geometryColor * Kd;

      if (m->volume) {
        const float sample = m->volume->computeSample(m->volume, dg.P);
        geometryColor = geometryColor * m->volume->transferFunction->getColorForValue(m->volume->transferFunction, sample);
      }
    }
  }

  // Opacity of the geometry.
  float geometryOpacity = dg.color.w;

  // Compute lighting.
  vec3f shadedColor = make_vec3f(0.f);
  const vec2f s = make_vec2f(0.5f);

  for (uniform uint32 i=0; i<self->numLights; i++) {
    const Light_SampleRes light = self->lights[i]->sample(self->lights[i], dg, s);
    const float cosNL = abs(dot(safe_normalize(light.dir), dg.Ns));

    shadedColor = shadedColor + geometryColor * cosNL * light.weight;
  }

  // Set the color contribution for this sample only (do not accumulate).
  color = geometryOpacity * make_vec4f(shadedColor.x, shadedColor.y, shadedColor.z, 1.f);
}

/*! Returns the first hit volume for the provided ray and sets the ray bounds t0 and t,
    considering the provided ray offset and any clipping. If no volume is found, the
    returned volume is NULL and ray.t0 will be set to infinity. */
inline Volume *
RaycastVolumeRenderer_intersectVolumes(uniform RaycastVolumeRenderer *uniform self,
                                       varying Ray &ray,
                                       const uniform PassInfo &passInfo,
                                       const varying float &rayOffset)
{
  // The first intersected volume.
  Volume * volume = NULL;

  // The ray with bounds for the first intersected volume.
  Ray volumeRay = ray;
  volumeRay.t0 = infinity;

#if EXP_DATA_PARALLEL
  if (passInfo.block != NULL) {
    // Intersect volume bounding box.
    float t0, t1;
    Volume *uniform volume_i = passInfo.block->ispcVolume;
    intersectBox(ray, volume_i->boundingBox, t0, t1);
    t0 = max(t0,passInfo.region.lower);
    t1 = min(t1,passInfo.region.upper);

    if (t0 < t1 && t0 < volumeRay.t0) {
      volumeRay.t0 = t0;
      volumeRay.t = t1;
      volume = volume_i;
    }
    // Update the provided ray.
    ray = volumeRay;

    // If we intersected a volume, offset ray by a fraction of the nominal ray step.
    /* make sure that we place the first sample in this block exactly
       where it would have been if this was not its own block, but
       rather a region in a larger volume; else we get artifacts */
    if (volume) {
      float dt = volume->samplingStep * rcpf(volume->samplingRate);
      float t0 = ray.t0;
      int i0 = (int)(ray.t0 / dt);
      ray.t0 = (i0 + rayOffset)*dt;
      if (ray.t0 < t0) ray.t0 += dt;
    }
    // if (volume) ray.t0 += rayOffset * volume->samplingStep * rcpf(volume->samplingRate);

    // Return the first intersected volume.
    return volume;
  }
#endif

  // Test each volume and find the first intersection.
  for (uniform int32 i=0; i<self->super.model->volumeCount; i++) {
    Volume *uniform volume_i = self->super.model->volumes[i];
#if EXP_DATA_PARALLEL
    // hack for now .... if this does not have a 'getvoxel' function
    // it must be a data parallel volume...
    if (volume_i->computeSample == NULL) {
      // print("skipping data parallel piece...\n");
      // do NOT render data-parallel volumes in regular sampling mode...
      continue;
    }
#endif

    // Intersect volume bounding box.
    float t0, t1;
    intersectBox(ray, volume_i->boundingBox, t0, t1);

#if EXP_DATA_PARALLEL
    t0 = max(t0,passInfo.region.lower);
    t1 = min(t1,passInfo.region.upper);
#endif

    // Clip against volume clipping box (if specified).
    if(ne(volume_i->volumeClippingBox.lower, volume_i->volumeClippingBox.upper)) {
      float tClip0, tClip1;
      intersectBox(ray, volume_i->volumeClippingBox, tClip0, tClip1);

      t0 = max(t0, tClip0);
      t1 = min(t1, tClip1);
    }

    // Update intersected volume.
    if (t0 < t1 && t0 < volumeRay.t0) {
      volumeRay.t0 = t0;
      volumeRay.t = t1;
      volume = volume_i;
    }
  }

  // Update the provided ray.
  ray = volumeRay;

  // If we intersected a volume, offset ray by a fraction of the nominal ray
  // step.
  if (volume)
    ray.t0 += rayOffset * volume->samplingStep * rcpf(volume->samplingRate);

  // Return the first intersected volume.
  return volume;
}

/*! This function intersects the volume and geometries. */
void RaycastVolumeRenderer_intersect(uniform RaycastVolumeRenderer *uniform self,
                                     varying Ray &ray,
                                     const uniform PassInfo &passInfo,
                                     const varying float &rayOffset,
                                     varying vec4f &color,
                                     varying float &depth)
{
  const region1f clipRange = passInfo.region;

  if (clipRange.lower != 0.f) {
    ray.t0 = max(ray.t0,clipRange.lower);
    ray.t = min(ray.t,clipRange.upper);
  }

  // Original tMax for ray interval
  const float tMax = ray.t;

  // Ray epsilon based on bounding box of all volumes.
  uniform box3f boundingBox = self->super.model->volumeCount
    ? self->super.model->volumes[0]->boundingBox
    : make_box3f(make_vec3f(0.f), make_vec3f(0.f));

  Model *uniform model = self->super.model;
  for (uniform int32 i=1; i<self->super.model->volumeCount; i++) {
    boundingBox.lower = min(boundingBox.lower, model->volumes[i]->boundingBox.lower);
    boundingBox.upper = max(boundingBox.upper, model->volumes[i]->boundingBox.upper);
  }

  const uniform float epsilon
    = self->super.model->volumeCount
    ? 1e-4f * distance(boundingBox.lower, boundingBox.upper)
    : 1e-4f;

  // Copy of the ray for geometry intersection. The original ray is
  // used for volume intersection.
  Ray geometryRay = ray;
  geometryRay.primID = -1;
  geometryRay.geomID = -1;
  geometryRay.instID = -1;

  // Get first intersected volume for each ray and set the ray bounds.
  Volume *volume = RaycastVolumeRenderer_intersectVolumes(self, ray,
                                                          passInfo,
                                                          rayOffset);

  // Provide ray offset for use with isosurface geometries (this value ignored elsewhere).
  if (volume != NULL)
    geometryRay.time = -rayOffset * volume->samplingStep;

  // Separate color contributions for the volume and geometries. Initialize to provided color in case of no contribution...
  vec4f volumeColor = color;
  vec4f geometryColor = color;

  // Initial trace through geometries.
  RaycastVolumeRenderer_computeGeometrySample(self, geometryRay,
                                              clipRange, geometryColor);

  // Depth is the first volume bounding box or geometry hit
  depth = min(ray.t0, geometryRay.t);

  // Trace the ray through the volume and geometries.
  float firstHit;

  while ((firstHit = min(ray.t0, geometryRay.t)) < min(tMax,clipRange.upper)
         && min(min(color.x, color.y), color.z) < 1.0f && color.w < 0.99f) {
    if (firstHit == ray.t0) {

      // Check to see if we've exited the current volume.
      if (ray.t0 >= ray.t) {

        ray.t0 = ray.t + epsilon;
        ray.t = tMax;
        volume = RaycastVolumeRenderer_intersectVolumes(self, ray, passInfo, rayOffset);
      }
      else {

        if (any(volume == NULL))
          print("ACCESSING NULL VOLUME HERE!\n");

        // Compute the volume sample at the current position and advance the ray.
        foreach_unique (v in volume)
          RaycastVolumeRenderer_computeVolumeSample(self, v, ray, volumeColor);

        // Volume contribution.
        color = color + (1.0f - color.w) * volumeColor;
      }
    }
    else if (firstHit == geometryRay.t) {

      // Geometry contribution.
      color = color + (1.0f - color.w) * geometryColor;

      // Reset geometry ray.
      geometryRay.t0 = geometryRay.t + epsilon;
      geometryRay.t = tMax; //!< end of valid ray interval for traceRay()
      geometryRay.primID = -1;
      geometryRay.geomID = -1;
      geometryRay.instID = -1;

      // Update ray offset for use with isosurface geometries based on current volume (this value ignored elsewhere).
      geometryRay.time = volume ? -rayOffset * volume->samplingStep : 0.f;

      // Trace next geometry ray.
      RaycastVolumeRenderer_computeGeometrySample(self, geometryRay,
                                                  clipRange,
                                                  geometryColor);
    }
  }
}

void RaycastVolumeRenderer_renderSample(Renderer *uniform pointer,
                                        void *uniform _passInfo,
                                        varying ScreenSample &sample)
{
  RaycastVolumeRenderer *uniform self
    = (RaycastVolumeRenderer *uniform) pointer;

  PassInfo *uniform passInfo
    = (PassInfo *uniform)_passInfo;

  // Background color.
  const uniform bool useBG = ((passInfo == NULL) || passInfo->useBG);

  // Ray offset for this sample, as a fraction of the nominal step size.
  float rayOffset = precomputedHalton2(sample.sampleID.z);
  int ix = sample.sampleID.x % 4;
  int iy = sample.sampleID.y % 4;
  int patternID = ix + 4 * iy;
  rayOffset += precomputedHalton3(patternID);

  if(rayOffset > 1.f) rayOffset -= 1.f;

  // Provide the self to the intersector as it contains all
  // volumes, geometries, etc.
  vec4f color = make_vec4f(0.0f,0.f,0.f,0.f);
  float depth = infinity;

  if (passInfo != NULL) {
    RaycastVolumeRenderer_intersect(self, sample.ray,
                                    *passInfo, rayOffset, color, depth);
  } else {
    uniform PassInfo dummyPassInfo;
    dummyPassInfo.region = make_region1f(0.f,inf);
    dummyPassInfo.useBG = true;
    dummyPassInfo.block = NULL;
    RaycastVolumeRenderer_intersect(self, sample.ray,
                                    dummyPassInfo, rayOffset, color, depth);
  }

  // Attenuate the foreground and background colors by the opacity.
  if (useBG && self->super.backgroundEnabled) {
    const vec4f background = make_vec4f(self->super.bgColor, 1.f);
    color = color.w * color + (1.0f - color.w) * background;
  }

  // Store the result in the sample.
  sample.rgb.x = color.x;
  sample.rgb.y = color.y;
  sample.rgb.z = color.z;
  sample.alpha = color.w;
  sample.z = depth;
}

export void *uniform RaycastVolumeRenderer_createInstance()
{
  RaycastVolumeRenderer *uniform self = uniform new uniform RaycastVolumeRenderer;

  Renderer_Constructor(&self->super, NULL);
  self->super.renderSample = RaycastVolumeRenderer_renderSample;

  return self;
}

export void RaycastVolumeRenderer_setLights(void *uniform _self,
                                            void **uniform lights,
                                            const uniform uint32 numLights)
{
  // Cast to the actual Renderer subtype.
  uniform RaycastVolumeRenderer *uniform self = (uniform RaycastVolumeRenderer *uniform)_self;

  // Set the light sources.
  self->lights = (Light **uniform)lights;
  self->numLights = numLights;
}

#if EXP_DATA_PARALLEL
extern "C" Tile *uniform CacheForBlockTiles_getTileForBlock(void *uniform _tileCache,
                                                            uniform uint32 blockID);

inline uniform bool itIsIThatHasToRenderThisBlock(DDBVolumeBlock *uniform block,
                                                  uniform int32 tileID,
                                                  uniform int32 myRank)
{
  return
    block->mine
    &&
    (myRank - block->firstOwner) == (tileID % block->numOwners);
}

inline void DDVR_renderSample(RaycastVolumeRenderer *uniform self,
                              ScreenSample &fgSample,
                              ScreenSample &bgSample,
                              void *uniform _tileCache,
                              uint32 pixelID,
                              uniform int numBlocks,
                              DDBVolumeBlock *uniform block,
                              bool *uniform tileNeedsBlock,
                              uniform int32 tileID,
                              uniform int32 myRank,
                              uniform bool isMyTile)
{
  // ray interval that overlaps _any_ block
  region1f blockRegion = make_region1f(+inf,-inf);
  float org_ray_t0 = fgSample.ray.t0;
  float org_ray_t1 = fgSample.ray.t;
  for (uniform int blockID=0;blockID<numBlocks;blockID++) {
    float t0 = org_ray_t0;
    float t1 = org_ray_t1;
    intersectBox(fgSample.ray,block[blockID].bounds,t0,t1);
    if (t0 >= t1) {
      // ray does not intersect this block...
    } else {
      if (!tileNeedsBlock[blockID]) tileNeedsBlock[blockID] = true;
      blockRegion.lower = min(blockRegion.lower,t0);
      blockRegion.upper = max(blockRegion.upper,t1);

      // now, render this block if it's one of ours
      if (!block[blockID].mine) continue;
      if (itIsIThatHasToRenderThisBlock(&block[blockID],tileID,myRank)) {
        Tile *uniform blockTile = CacheForBlockTiles_getTileForBlock(_tileCache,blockID);
        // -------------------------------------------------------
        // set up pass for 'one particular block'
        // -------------------------------------------------------
        uniform PassInfo passInfo;
        // set integration pass up for region from (at most) ray origin to start of blocks
        passInfo.region = make_region1f(t0,t1);
        // do not use background color in this pass
        passInfo.useBG = false;
        // do not use any block in this pass
        passInfo.block = &block[blockID];
        RaycastVolumeRenderer_renderSample((Renderer*uniform)self,&passInfo,fgSample);
        fgSample.ray.t0 = org_ray_t0;
        fgSample.ray.t  = org_ray_t1;
        setRGBAZ(*blockTile,pixelID,fgSample.rgb,fgSample.alpha,fgSample.z);
      }
    }
  }

if (!isMyTile) return;

  if (blockRegion.lower >= blockRegion.upper) {
    // ------------------------------------------------------------------
    // we're not intersecting any blocks.  --> render one sample for
    // entire ray interval (including background color), report it as
    // a single front sample, and return a dummy back sample. Note we
    // report it as a _front_ sample (even though it does have the
    // background in it) because that way tiles that ahve some such
    // samples in it will not report close geometry in a 'back' tile,
    // (which messes up the tile's minimum distance).
    // ------------------------------------------------------------------

    uniform PassInfo passInfo;
    // set integration pass up for region from (at most) ray origin to start of blocks
    passInfo.region = make_region1f(fgSample.ray.t0,fgSample.ray.t);
    // do not use background color in this pass
    passInfo.useBG = true;
    // do not use any block in this pass
    passInfo.block = NULL;

    // ray does not overlap _any_ block; render entire ray segment in one sweep
    RaycastVolumeRenderer_renderSample((Renderer*uniform)self,&passInfo,fgSample);

    bgSample.rgb = make_vec3f(0.f,0.f,0.f);
    bgSample.z = inf;
    bgSample.alpha = 0.f;
  } else {
    // ray overlaps the data parallel volume. render both the region
    // BEFORE, and AFTER the dp volume, but not yet anything inside.

    // -------------------------------------------------------
    // set up pass for 'before any blocks'
    // -------------------------------------------------------
    uniform PassInfo passInfo;
    // set integration pass up for region from (at most) ray origin to start of blocks
    passInfo.region = make_region1f(fgSample.ray.t0,blockRegion.lower);
    // do not use background color in this pass
    passInfo.useBG = false;
    // do not use any block in this pass
    passInfo.block = NULL;
    RaycastVolumeRenderer_renderSample((Renderer*uniform)self,&passInfo,fgSample);

    // -------------------------------------------------------
    // set up pass for 'after any blocks'
    // -------------------------------------------------------
    // set integration pass up for region from (at most) ray origin to start of blocks
    passInfo.region = make_region1f(blockRegion.upper,bgSample.ray.t);
    // do not use background color in this pass
    passInfo.useBG = true;
    // do not use any block in this pass
    passInfo.block = NULL;
    RaycastVolumeRenderer_renderSample((Renderer*uniform)self,&passInfo,bgSample);
  }
}

export void DDDVRRenderer_renderTile(void*uniform _self,
                                     uniform Tile &fgTile,
                                     uniform Tile &bgTile,
                                     void *uniform _tileCache,
                                     uniform int numBlocks,
                                     void *uniform _block,
                                     uniform bool *uniform tileNeedsBlock,
                                     uniform int32 tileID,
                                     uniform int32 myRank,
                                     uniform bool isMyTile,
                                     uniform int taskIndex)
{
  uniform Renderer *uniform self = (uniform Renderer *uniform)_self;
  DDBVolumeBlock *uniform block = (DDBVolumeBlock *uniform)_block;

  uniform FrameBuffer *uniform fb     = self->fb;
  uniform Camera      *uniform camera = self->camera;

  float pixel_du = .5f, pixel_dv = .5f;
  float lens_du = 0.f,  lens_dv = 0.f;

  ScreenSample fgSample, bgSample;
  fgSample.z = inf;
  fgSample.alpha = 0.f;

  CameraSample cameraSample;

  const uniform int begin = taskIndex * RENDERTILE_PIXELS_PER_JOB;
  const uniform int end   = begin     + RENDERTILE_PIXELS_PER_JOB;
  const uniform int startSampleID = max(fgTile.accumID,0);

  for (uniform uint32 i=begin;i<end;i+=programCount) {
    const uint32 index = i + programIndex;
    fgSample.sampleID.x        = bgTile.region.lower.x + z_order.xs[index];
    fgSample.sampleID.y        = bgTile.region.lower.y + z_order.ys[index];

    if ((fgSample.sampleID.x >= fb->size.x) |
        (fgSample.sampleID.y >= fb->size.y))
      continue;

    const uint32 pixel = z_order.xs[index] + (z_order.ys[index] * TILE_SIZE);

    pixel_du = precomputedHalton2(startSampleID);
    pixel_dv = precomputedHalton3(startSampleID);
    fgSample.sampleID.z = startSampleID;

    cameraSample.screen.x = (fgSample.sampleID.x + pixel_du) * fb->rcpSize.x;
    cameraSample.screen.y = (fgSample.sampleID.y + pixel_dv) * fb->rcpSize.y;

    camera->initRay(camera,fgSample.ray,cameraSample);

    // set ray t value for early ray termination if we have a maximum depth texture
    if (self->maxDepthTexture) {
      // always sample center of pixel
      vec2f depthTexCoord;
      depthTexCoord.x = (fgSample.sampleID.x + 0.5f) * fb->rcpSize.x;
      depthTexCoord.y = (fgSample.sampleID.y + 0.5f) * fb->rcpSize.y;

      const float oldT = fgSample.ray.t;
      fgSample.ray.t = max(get1f(self->maxDepthTexture, depthTexCoord),oldT);
    }

    bgSample = fgSample;
    DDVR_renderSample((RaycastVolumeRenderer*uniform)self,
                      fgSample,bgSample,_tileCache,pixel,
                      numBlocks,block,tileNeedsBlock,tileID,myRank,isMyTile);

    setRGBAZ(bgTile,pixel,bgSample.rgb,bgSample.alpha,bgSample.z);
    setRGBAZ(fgTile,pixel,fgSample.rgb,fgSample.alpha,fgSample.z);
  }
}

#endif
