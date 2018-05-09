// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "SciVisRenderer.ih"
#include "fb/FrameBuffer.ih"
#include "render/util.ih"
#include "texture/Texture2D.ih"
#include "math/random.ih"
#include "math/sampling.ih"
#include "math/LinearSpace.ih"
#include "surfaceShading.ih"
#include "SciVisMaterial.ih"

// Data types /////////////////////////////////////////////////////////////////

// Function definitions ///////////////////////////////////////////////////////

vec4f SciVisRenderer_computeGeometrySample(SciVisRenderer *uniform self,
                                           const varying vec3i &sampleID,
                                           varying Ray &ray, const varying float &rayOffset)
{
  vec3f color        = make_vec3f(0.f);
  float path_opacity = 1.f;
  uniform int path_depth = 0;
  float first_hit_dist = 0.f;

  while (1) {

    traceRay(self->super.model, ray);

    // Check if we missed, if so we are done //
    if (ray.geomID < 0) {
      return make_vec4f(color, 1.f - path_opacity);
    }

    // Record depth of first hit for depth output //
    if (path_depth == 0) {
      first_hit_dist = ray.t;
    }

    // Start shading //

    // Calculate material information from DG //
    DifferentialGeometry dg;
    postIntersect(self->super.model,
                  dg,
                  ray,
                  DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_TANGENTS|
                  DG_MATERIALID|DG_COLOR|DG_TEXCOORD);

    SciVisShadingInfo info;
    initShadingInfo(info);

    shadeMaterials(dg, info);

    info.local_opacity = path_opacity * info.d;

    if (info.local_opacity > self->super.minContribution) { // worth shading?
      shadeAO(self, sampleID, dg, info, color);
      integrateOverLights(self, ray, dg, info, color, rayOffset,sampleID, 0.5f);
    }

    // Kill path when reached max depth or if remaining contribution too low
    path_opacity = path_opacity * (1.f - info.d);
    path_depth++;
    if (path_depth >= self->super.maxDepth
        || path_opacity < self->super.minContribution) {
      ray.t = first_hit_dist;
      return make_vec4f(color, 1.f - path_opacity);
    }

    // Reset ray
    ray.t0 = ray.t + self->super.epsilon;
    ray.t = infinity;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }
}

/*! This function intersects the volume and geometries. */
void SciVisRenderer_intersect(uniform SciVisRenderer *uniform renderer,
                              varying Ray &ray,
                              const varying float &rayOffset,
                              const varying vec3i &sampleID,
                              varying vec4f &color,
                              varying float &depth)
{
  // Original tMax for ray interval
  const float tMax = ray.t;

  // Copy of the ray for geometry intersection. The original ray is
  // used for volume intersection.
  Ray geometryRay = ray;
  geometryRay.primID = -1;
  geometryRay.geomID = -1;
  geometryRay.instID = -1;

  // Get first intersected volume for each ray and set the ray bounds.
  Volume *volume = SciVisRenderer_intersectVolumes(renderer, ray, rayOffset);

  // Provide ray offset for use with isosurface geometries (this value
  // ignored elsewhere).
  if (volume != NULL)
    geometryRay.time = -rayOffset * volume->samplingStep;

  // Initial trace through geometries.
  vec4f geometryColor = SciVisRenderer_computeGeometrySample(renderer,
                                                             sampleID, geometryRay, rayOffset);

  // Depth is the first volume bounding box or geometry hit
  depth = min(ray.t0, geometryRay.t);

  // Trace the ray through the volume and geometries.
  float firstHit;

  if(geometryRay.t < ray.t0 || volume == NULL) {
    // Geometry contribution.
    color = geometryColor;
  }

  while ((firstHit = min(ray.t0, geometryRay.t)) < tMax
         /*&& min(min(color.x, color.y), color.z) < 1.0f*///<--why is this here?
         && color.w < 0.99f) {

    if (firstHit == ray.t0) {

      // Check to see if we've exited the current volume.
      if (ray.t0 >= ray.t) {

        ray.t0 = ray.t + renderer->volumeEpsilon;
        ray.t = tMax;
        volume = SciVisRenderer_intersectVolumes(renderer, ray, rayOffset);

      } else {

        if (any(volume == NULL))
          print("ACCESSING NULL VOLUME HERE!\n");

        // Compute the volume sample at the current position and advance the ray
        vec4f volumeColor;
        float tBegin = ray.t0;
        float tEnd = min(geometryRay.t,ray.t);
        foreach_unique (v in volume)
          {
            //interval sampling
            volumeColor = SciVisRenderer_computeVolumeInterval(renderer, v, ray, tBegin, tEnd, .99f, 0, rayOffset, sampleID, 1.f);
          }

        // Volume contribution.
        color = color + (1.0f - color.w) * volumeColor;
        ray.t0 = tEnd + renderer->volumeEpsilon;
      }

    } else {// firstHit == geometryRay.t

      // Geometry contribution.
      color = color + (1.0f - color.w) * geometryColor;

      if (color.w < 0.99f) {
        // Reset geometry ray.
        geometryRay.t0 = geometryRay.t + renderer->volumeEpsilon;
        geometryRay.t = tMax; //!< end of valid ray interval for traceRay()
        geometryRay.primID = -1;
        geometryRay.geomID = -1;
        geometryRay.instID = -1;

        // Update ray offset for use with isosurface geometries based on current
        // volume (this value ignored elsewhere).
        geometryRay.time = volume ? -rayOffset * volume->samplingStep : 0.f;

        // Trace next geometry ray.
        geometryColor = SciVisRenderer_computeGeometrySample(renderer,
                                                             sampleID, geometryRay, rayOffset);
      }

    }
  }
}

void SciVisRenderer_renderSample(uniform Renderer *uniform _self,
                                 void *uniform perFrameData,
                                 varying ScreenSample &sample)
{
  SciVisRenderer *uniform renderer
    = (SciVisRenderer *uniform) _self;

  // Ray offset for this sample, as a fraction of the nominal step size.
  float rayOffset = precomputedHalton2(sample.sampleID.z);
  int ix = sample.sampleID.x % 4;
  int iy = sample.sampleID.y % 4;
  int patternID = ix + 4 * iy;
  rayOffset += precomputedHalton3(patternID);

  if(rayOffset > 1.f) rayOffset -= 1.f;

  // Provide the renderer to the intersector as it contains all
  // volumes, geometries, etc.
  vec4f color = make_vec4f(0.0f);
  float depth = infinity;

  SciVisRenderer_intersect(renderer, sample.ray, rayOffset,
                           sample.sampleID, color, depth);

  // blend with background
  color = color + (1.0f - color.w) * renderer->super.bgColor;

  // Store the result in the sample.
  sample.rgb = make_vec3f(color);
  sample.alpha = color.w;
  sample.z = depth;
}

static unmasked void *uniform
SciVisRenderer_beginFrame(uniform Renderer *uniform _self,
                          uniform FrameBuffer *uniform fb)
{
  uniform SciVisRenderer *uniform self = (uniform SciVisRenderer *uniform)_self;
  self->super.fb = fb;
  Model *uniform model = self->super.model;
  self->volumeEpsilon = self->super.epsilon;

  if (model->volumeCount) {
    // Ray epsilon based on bounding box of all volumes.
    uniform box3f boundingBox = make_box3f_empty();
    for (uniform int32 i = 0; i < model->volumeCount; i++)
      boundingBox = box_extend(boundingBox, model->volumes[i]->boundingBox);

    self->volumeEpsilon = 1e-3f * length(box_size(boundingBox));
  }

  return NULL;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void SciVisRenderer_set(void *uniform _self,
                               const uniform bool shadowsEnabled,
                               const uniform int aoSamples,
                               const uniform float aoDistance,
                               const uniform vec3f &aoColor,
                               const uniform bool aoTransparencyEnabled,
                               void **uniform lights,
                               const uniform uint32 numLights,
                               const uniform bool oneSidedLighting
                               )
{
  uniform SciVisRenderer *uniform self = (uniform SciVisRenderer *uniform)_self;

  self->shadowsEnabled = shadowsEnabled;
  self->aoSamples = aoSamples;
  self->aoDistance = aoDistance;
  self->aoTransparencyEnabled = aoTransparencyEnabled;

  // already factor in parts of cosineSampleHemispherePDF
  self->aoColor = aoColor * pi;

  self->lights = (const uniform Light *uniform *uniform)lights;
  self->numLights = numLights;
  self->oneSidedLighting = oneSidedLighting;


  // pre-computations:
  self->needToDoAO
    =  (self->aoSamples > 0)
    && (reduce_max(self->aoColor) > 0.f)
    && (self->aoDistance > 0.f);
}

export void *uniform SciVisRenderer_create(void *uniform cppE)
{
  uniform SciVisRenderer *uniform self = uniform new uniform SciVisRenderer;
  Renderer_Constructor(&self->super,cppE);
  self->super.renderSample = SciVisRenderer_renderSample;
  self->super.beginFrame = SciVisRenderer_beginFrame;
  SciVisRenderer_set(self, false, 4, infinity, make_vec3f(0.25f), false, NULL, 0, true);

  return self;
}
