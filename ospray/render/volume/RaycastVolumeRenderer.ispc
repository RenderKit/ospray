// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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
#include "ospray/render/volume/RaycastVolumeRendererMaterial.ih"

void RaycastVolumeRenderer_renderFramePostamble(Renderer *uniform renderer, 
                                                const uniform int32 accumID)
{ 
  if (renderer->fb) renderer->fb->accumID = accumID;  renderer->fb = NULL; 
}

void RaycastVolumeRenderer_renderFramePreamble(Renderer *uniform renderer, 
                                               FrameBuffer *uniform framebuffer)
{ 
  renderer->fb = framebuffer; 
}

inline void RaycastVolumeRenderer_computeVolumeSample(RaycastVolumeRenderer *uniform renderer,
                                                      Volume *uniform volume,
                                                      varying Ray &ray,
                                                      varying vec4f &color)
{
  // Sample the volume at the hit point in world coordinates.
  const vec3f coordinates = ray.org + ray.t0 * ray.dir;
  const float sample = volume->computeSample(volume, coordinates);

  // Look up the color associated with the volume sample.
  vec3f sampleColor = volume->transferFunction->getColorForValue(volume->transferFunction, sample);

  // Compute gradient shading, if enabled.
  if(volume->gradientShadingEnabled) {

    // Compute lighting.
    const vec2f s = make_vec2f(0.5f);
    DifferentialGeometry dg;
    dg.P = coordinates;
    LightSample light = renderer->lights[0]->sample(renderer->lights[0], dg, s);

    // Constant ambient lighting term.
    const float ambient = 0.1f;

    // Use volume gradient as the normal.
    const vec3f gradient = normalize(volume->computeGradient(volume, coordinates));

    const float cosNL = isnan(gradient.x+gradient.y+gradient.z) ? 1.f : abs(dot(normalize(light.direction), normalize(gradient)));

    sampleColor = sampleColor * (ambient + cosNL*(1.f-ambient)) * light.radiance;
  }

  // Look up the opacity associated with the volume sample.
  const float sampleOpacity = volume->transferFunction->getOpacityForValue(volume->transferFunction, sample);

  // Set the color contribution for this sample only (do not accumulate).
  color = clamp(sampleOpacity / volume->samplingRate) * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);

  // Advance the ray for the next sample.
  volume->intersect(volume, ray);
}

inline void RaycastVolumeRenderer_computeGeometrySample(RaycastVolumeRenderer *uniform renderer,
                                                        varying Ray &ray,
                                                        varying vec4f &color)
{
  // We compute intersections on the model and provide the contribution for the closest hit.
  traceRay(renderer->inherited.model, ray);

  // No hit found.
  if(ray.geomID < 0) {
    ray.t = infinity;
    return;
  }

  // Post intersect on the hit geometry.
  DifferentialGeometry dg;
  postIntersect(renderer->inherited.model, dg, ray, DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_COLOR|DG_MATERIALID);

  // Color of the geometry.
  vec3f sampleColor = make_vec3f(dg.color.x, dg.color.y, dg.color.z);

  // Apply material if it exists.
  uniform RaycastVolumeRendererMaterial *mat = (uniform RaycastVolumeRendererMaterial*)dg.material;

  if (mat) {
    foreach_unique(m in mat) {
      sampleColor = sampleColor * m->Kd;

      if (m->volume) {
        const float sample = m->volume->computeSample(m->volume, dg.P);
        sampleColor = sampleColor * m->volume->transferFunction->getColorForValue(m->volume->transferFunction, sample);
      }
    }
  }

  // Opacity of the geometry.
  float sampleOpacity = dg.color.w;

  // Compute lighting.
  const vec2f s = make_vec2f(0.5f);
  LightSample light = renderer->lights[0]->sample(renderer->lights[0], dg, s);

  // Constant ambient lighting term.
  const float ambient = 0.1f;

  const float cosNL = abs(dot(normalize(light.direction), dg.Ns));

  sampleColor = sampleColor * (ambient + cosNL*(1.f-ambient)) * light.radiance;

  // Set the color contribution for this sample only (do not accumulate).
  color = sampleOpacity * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.f);
}

/*! Returns the first hit volume for the provided ray and sets the ray bounds t0 and t,
    considering the provided ray offset and any clipping. If no volume is found, the
    returned volume is NULL and ray.t0 will be set to infinity. */
inline Volume * RaycastVolumeRenderer_intersectVolumes(uniform RaycastVolumeRenderer *uniform renderer,
                                                       varying Ray &ray,
                                                       const varying float &rayOffset)
{
  // The first intersected volume.
  Volume * volume = NULL;

  // The ray with bounds for the first intersected volume.
  Ray volumeRay = ray;
  volumeRay.t0 = infinity;

  // Test each volume and find the first intersection.
  for (uniform int32 i=0; i<renderer->inherited.model->volumeCount; i++) {

    // Intersect volume bounding box.
    float t0, t1;
    intersectBox(ray, renderer->inherited.model->volumes[i]->boundingBox, t0, t1);

    // Clip against volume clipping box (if specified).
    if(ne(renderer->inherited.model->volumes[i]->volumeClippingBox.lower, renderer->inherited.model->volumes[i]->volumeClippingBox.upper)) {
      float tClip0, tClip1;
      intersectBox(ray, renderer->inherited.model->volumes[i]->volumeClippingBox, tClip0, tClip1);

      t0 = max(t0, tClip0);
      t1 = min(t1, tClip1);
    }

    // Update intersected volume.
    if (t0 < t1 && t0 < volumeRay.t0) {
      volumeRay.t0 = t0;
      volumeRay.t = t1;
      volume = renderer->inherited.model->volumes[i];
    }
  }

  // Update the provided ray.
  ray = volumeRay;

  // If we intersected a volume, offset ray by a fraction of the nominal ray step.
  if (volume) ray.t0 += rayOffset * volume->samplingStep * rcpf(volume->samplingRate);

  // Return the first intersected volume.
  return volume;
}

/*! This function intersects the volume and geometries. */
inline void RaycastVolumeRenderer_intersect(uniform RaycastVolumeRenderer *uniform renderer,
                                            varying Ray &ray,
                                            const varying float &rayOffset,
                                            varying vec4f &color)
{
  // Original tMax for ray interval
  const float tMax = ray.t;

  // Ray epsilon based on bounding box of all volumes.
  uniform box3f boundingBox = renderer->inherited.model->volumeCount ? renderer->inherited.model->volumes[0]->boundingBox :
                                                                       make_box3f(make_vec3f(0.f), make_vec3f(0.f));

  for (uniform int32 i=1; i<renderer->inherited.model->volumeCount; i++) {
    boundingBox.lower = min(boundingBox.lower, renderer->inherited.model->volumes[i]->boundingBox.lower);
    boundingBox.upper = max(boundingBox.upper, renderer->inherited.model->volumes[i]->boundingBox.upper);
  }

  const uniform float epsilon = renderer->inherited.model->volumeCount ? 1e-4f * distance(boundingBox.lower, boundingBox.upper) :
                                                                         1e-4f;

  // Copy of the ray for geometry intersection. The original ray is used for volume intersection.
  Ray geometryRay = ray;
  geometryRay.primID = -1;
  geometryRay.geomID = -1;
  geometryRay.instID = -1;

  // Get first intersected volume for each ray and set the ray bounds.
  Volume *volume = RaycastVolumeRenderer_intersectVolumes(renderer, ray, rayOffset);

  // Provide ray offset for use with isosurface geometries (this value ignored elsewhere).
  if (volume)
    geometryRay.time = -rayOffset * volume->samplingStep;

  // Separate color contributions for the volume and geometries. Initialize to provided color in case of no contribution...
  vec4f volumeColor = color;
  vec4f geometryColor = color;

  // Initial trace through geometries.
  RaycastVolumeRenderer_computeGeometrySample(renderer, geometryRay, geometryColor);

  // Trace the ray through the volume and geometries.
  float firstHit;

  while ((firstHit = min(ray.t0, geometryRay.t)) < tMax
         && min(min(color.x, color.y), color.z) < 1.0f && color.w < 0.99f) {

    if (firstHit == ray.t0) {

      // Check to see if we've exited the current volume.
      if (ray.t0 >= ray.t) {

        ray.t0 = ray.t + epsilon;
        ray.t = tMax;
        volume = RaycastVolumeRenderer_intersectVolumes(renderer, ray, rayOffset);
      }
      else {

        // Compute the volume sample at the current position and advance the ray.
        foreach_unique (v in volume)
          RaycastVolumeRenderer_computeVolumeSample(renderer, v, ray, volumeColor);

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
      RaycastVolumeRenderer_computeGeometrySample(renderer, geometryRay, geometryColor);
    }
  }
}

void RaycastVolumeRenderer_renderSample(Renderer *uniform pointer, 
                                        varying ScreenSample &sample) 
{
  // Cast to the actual Renderer subtype.
  RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

  // Background color.
  const vec4f background = make_vec4f(renderer->bgColor.x, renderer->bgColor.y, renderer->bgColor.z, 1.f);

  // Ray offset for this sample, as a fraction of the nominal step size.
  float rayOffset = precomputedHalton2(sample.sampleID.z);
  int ix = sample.sampleID.x % 4;
  int iy = sample.sampleID.y % 4;
  int patternID = ix + 4 * iy;
  rayOffset += precomputedHalton3(patternID);

  if(rayOffset > 1.f) rayOffset -= 1.f;

  // Provide the renderer to the intersector as it contains all volumes, geometries, etc.
  vec4f color = make_vec4f(0.0f);
  RaycastVolumeRenderer_intersect(renderer, sample.ray, rayOffset, color);

  // Attenuate the foreground and background colors by the opacity.
  if (renderer->inherited.backgroundEnabled)
    color = color.w * color + (1.0f - color.w) * background;

  // Store the result in the sample.
  sample.rgb.x = color.x;  sample.rgb.y = color.y;  sample.rgb.z = color.z;  sample.alpha = color.w;
}

export void *uniform RaycastVolumeRenderer_createInstance()
{
  // The renderer object.
  RaycastVolumeRenderer *uniform renderer = uniform new uniform RaycastVolumeRenderer;

  // Constructor of the parent class.
  Renderer_Constructor(&renderer->inherited, NULL);

  // Function to compute the color and opacity for a screen space sample.
  renderer->inherited.renderSample = RaycastVolumeRenderer_renderSample;

  // Function to perform per-frame state initialization.
  renderer->inherited.beginFrame = RaycastVolumeRenderer_renderFramePreamble;

  // Function to perform per-frame state completion.
  renderer->inherited.endFrame = RaycastVolumeRenderer_renderFramePostamble;

  return renderer;
}

export void RaycastVolumeRenderer_setBackgroundColor(void *uniform _self,
                                                     const uniform vec3f &bgColor)
{
  // Cast to the actual Renderer subtype.
  uniform RaycastVolumeRenderer *uniform self = (uniform RaycastVolumeRenderer *uniform)_self;

  // Set the background color.
  self->bgColor = bgColor;
}

export void RaycastVolumeRenderer_setLights(void *uniform _self,
                                            void **uniform lights)
{
  // Cast to the actual Renderer subtype.
  uniform RaycastVolumeRenderer *uniform self = (uniform RaycastVolumeRenderer *uniform)_self;

  // Set the light sources.
  self->lights = (Light **uniform) lights;
}
