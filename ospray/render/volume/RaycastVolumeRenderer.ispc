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
  //! Advance the ray.
  volume->intersect(volume, ray);

  if (ray.t > ray.t1)
    return;

  //! Sample the volume at the hit point in world coordinates.
  const float sample = volume->computeSample(volume, ray.org + ray.t * ray.dir);

  //! Look up the color associated with the volume sample.
  vec3f sampleColor = volume->transferFunction->getColorForValue(volume->transferFunction, sample);

  //! Compute gradient shading, if enabled.
  if(volume->gradientShadingEnabled) {

    //! Compute lighting.
    vec3f lightDirection;
    float lightDistance;
    vec3f lightRadiance = renderer->lights[0]->computeRadiance(renderer->lights[0], ray.org + ray.t * ray.dir, lightDirection, lightDistance);

    //! Constant ambient lighting term.
    const float ambient = 0.1f;

    //! Use volume gradient as the normal.
    const vec3f gradient = normalize(volume->computeGradient(volume, ray.org + ray.t * ray.dir));

    const float cosNL = isnan(gradient.x+gradient.y+gradient.z) ? 1.f : abs(dot(normalize(lightDirection), normalize(gradient)));

    sampleColor = sampleColor * (ambient + cosNL*(1.f-ambient)) * lightRadiance;
  }

  //! Look up the opacity associated with the volume sample.
  const float sampleOpacity = volume->transferFunction->getOpacityForValue(volume->transferFunction, sample);

  //! Set the color contribution for this sample only (do not accumulate).
  color = clamp(sampleOpacity / volume->samplingRate) * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);
}

inline void RaycastVolumeRenderer_computeGeometrySample(RaycastVolumeRenderer *uniform renderer,
                                                        Volume *uniform volume,
                                                        varying Ray &ray,
                                                        varying vec4f &color)
{
  //! We compute intersections on the model and provide the contribution for the closest hit.
  traceRay(renderer->inherited.model, ray);

  //! No hit found.
  if(ray.geomID < 0) {
    ray.t = infinity;
    return;
  }

  //! Post intersect on the hit geometry.
  DifferentialGeometry dg;
  postIntersect(renderer->inherited.model, dg, ray, DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_COLOR);

  //! Color of the geometry.
  vec3f sampleColor = make_vec3f(dg.color.x, dg.color.y, dg.color.z);

  //! Opacity of the geometry.
  float sampleOpacity = dg.color.w;

  //! If no color found in the geometry, map through transfer function.
  if(sampleColor.x < 0.f) {

    //! Sample the volume.
    const float sample = volume->computeSample(volume, dg.P);

    //! Look up the color associated with the volume sample. NaN values will have a color of (0,0,0) and a 0 opacity.
    sampleColor = isnan(sample) ? sampleOpacity = 0.f, make_vec3f(0.f) : volume->transferFunction->getColorForValue(volume->transferFunction, sample);
  }

  //! Compute lighting.
  vec3f lightDirection;
  float lightDistance;
  vec3f lightRadiance = renderer->lights[0]->computeRadiance(renderer->lights[0], dg.P, lightDirection, lightDistance);

  //! Constant ambient lighting term.
  const float ambient = 0.1f;

  const float cosNL = abs(dot(normalize(lightDirection), dg.Ns));

  sampleColor = sampleColor * (ambient + cosNL*(1.f-ambient)) * lightRadiance;

  //! Set the color contribution for this sample only (do not accumulate).
  color = make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, sampleOpacity);
}

//! This function intersects the volume and geometries.
inline void RaycastVolumeRenderer_intersect(uniform RaycastVolumeRenderer *uniform renderer,
                                            varying Ray &ray,
                                            const varying float &rayOffset,
                                            varying vec4f &color)
{
  //! Assume just one volume.
  Volume *uniform volume = renderer->inherited.model->volumes[0];

  //! Bounding box of the volume.
  const uniform box3f boundingBox = volume->boundingBox;

  //! Ray epsilon.
  const uniform float epsilon = 1e-4 * distance(boundingBox.lower, boundingBox.upper);

  //! Copy of the ray for geometry intersection. The original ray is used for volume intersection.
  Ray geometryRay = ray;
  geometryRay.time = -rayOffset * volume->samplingStep; //!< used by isosurface geometries as a ray offset
  geometryRay.primID = -1;
  geometryRay.geomID = -1;
  geometryRay.instID = -1;

  //! Compute the intersection interval over the ray and volume bounds.
  intersectBox(ray, boundingBox, ray.t0, ray.t);

  //! And the volume clipping box bounds.
  if(ne(volume->volumeClippingBox.lower, volume->volumeClippingBox.upper))
    intersectBox(ray, volume->volumeClippingBox, ray.t0, ray.t);

  ray.t1 = ray.t;

  if (ray.t0 > ray.t1)
    ray.t = infinity;
  else
    ray.t = ray.t0;

  //! Offset ray by a fraction of the nominal ray step.
  const uniform float step = volume->samplingStep / volume->samplingRate;
  ray.t += rayOffset * step;

  //! Separate color contributions for the volume and geometries. Initialize to provided color in case of no contribution...
  vec4f volumeColor = color;
  vec4f geometryColor = color;

  //! Initial trace through the volume and geometries.
  RaycastVolumeRenderer_computeVolumeSample(renderer, volume, ray, volumeColor);
  RaycastVolumeRenderer_computeGeometrySample(renderer, volume, geometryRay, geometryColor);

  //! Trace the ray through the volume and geometries.
  float firstHit;

  while ((firstHit = min(ray.t, geometryRay.t)) < infinity
         && min(min(color.x, color.y), color.z) < 1.0f && color.w < 0.99f) {

    if (firstHit == ray.t) {

      //! Check to see if we've exited the volume.
      if (ray.t > ray.t1) {
        ray.t = infinity;
        continue;
      }

      //! Volume contribution.
      color = color + (1.0f - color.w) * volumeColor;

      //! Trace next volume ray.
      RaycastVolumeRenderer_computeVolumeSample(renderer, volume, ray, volumeColor);
    }
    else if (firstHit == geometryRay.t) {

      //! Geometry contribution.
      color = color + (1.0f - color.w) * geometryColor;

      //! Reset geometry ray.
      geometryRay.t0 = geometryRay.t + epsilon;
      geometryRay.t = infinity; //! end of valid ray interval for traceRay()
      geometryRay.primID = -1;
      geometryRay.geomID = -1;
      geometryRay.instID = -1;

      //! Trace next geometry ray.
      RaycastVolumeRenderer_computeGeometrySample(renderer, volume, geometryRay, geometryColor);
    }
  }
}

void RaycastVolumeRenderer_renderSample(Renderer *uniform pointer, 
                                        varying ScreenSample &sample) 
{
  //! Cast to the actual Renderer subtype.
  RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

  //! Background color.
  const vec4f background = make_vec4f(renderer->bgColor.x, renderer->bgColor.y, renderer->bgColor.z, 1.f);

  //! Ray offset for this sample, as a fraction of the nominal step size.
  float rayOffset = precomputedHalton2(sample.sampleID.z);
  int ix = sample.sampleID.x % 4;
  int iy = sample.sampleID.y % 4;
  int patternID = ix + 4 * iy;
  rayOffset += precomputedHalton3(patternID);

  if(rayOffset > 1.f) rayOffset -= 1.f;

  //! Provide the renderer to the intersector as it contains all volumes, geometries, etc.
  vec4f color = make_vec4f(0.0f);
  RaycastVolumeRenderer_intersect(renderer, sample.ray, rayOffset, color);

  //! Attenuate the foreground and background colors by the opacity.
  color = color.w * color + (1.0f - color.w) * background;

  //! Store the result in the sample.
  sample.rgb.x = color.x;  sample.rgb.y = color.y;  sample.rgb.z = color.z;  sample.alpha = color.w;
}

export void *uniform RaycastVolumeRenderer_createInstance()
{
  //! The renderer object.
  RaycastVolumeRenderer *uniform renderer = uniform new uniform RaycastVolumeRenderer;

  //! Constructor of the parent class.
  Renderer_Constructor(&renderer->inherited, NULL);

  //! Function to compute the color and opacity for a screen space sample.
  renderer->inherited.renderSample = RaycastVolumeRenderer_renderSample;

  //! Function to perform per-frame state initialization.
  renderer->inherited.beginFrame = RaycastVolumeRenderer_renderFramePreamble;

  //! Function to perform per-frame state completion.
  renderer->inherited.endFrame = RaycastVolumeRenderer_renderFramePostamble;

  return renderer;
}

export void RaycastVolumeRenderer_setBackgroundColor(void *uniform _self,
                                                     const uniform vec3f &bgColor)
{
  //! Cast to the actual Renderer subtype.
  uniform RaycastVolumeRenderer *uniform self = (uniform RaycastVolumeRenderer *uniform)_self;

  //! Set the background color.
  self->bgColor = bgColor;
}

export void RaycastVolumeRenderer_setLights(void *uniform _self,
                                            void **uniform lights)
{
  //! Cast to the actual Renderer subtype.
  uniform RaycastVolumeRenderer *uniform self = (uniform RaycastVolumeRenderer *uniform)_self;

  //! Set the light sources.
  self->lights = (Light **uniform) lights;
}
