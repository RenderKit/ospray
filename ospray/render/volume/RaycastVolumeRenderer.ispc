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
  volume->intersect(volume, ray);  if (ray.t > ray.t1) return;

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

inline void RaycastVolumeRenderer_computeIsosurfaceSample(RaycastVolumeRenderer *uniform renderer,
                                                          Volume *uniform volume,
                                                          varying Ray &ray,
                                                          varying vec4f &color)
{
  //! Terminate if there are no isovalues.
  if (volume->numIsovalues == 0) {
    ray.t = ray.t1 + 1.f;
    return;
  }

  //! Assume one isovalue for now.
  const uniform float isovalue = volume->isovalues[0];

  //! Advance the ray.
  volume->intersectIsosurface(volume, isovalue, ray);  if (ray.t > ray.t1) return;

  //! Sample the volume at the hit point in world coordinates.
  float t0 = ray.t;
  float sample0 = volume->computeSample(volume, ray.org + ray.t * ray.dir);

  while(1) {

    //! Advance the ray.
    volume->intersectIsosurface(volume, isovalue, ray);  if (ray.t > ray.t1) return;

    float t = ray.t;
    float sample = volume->computeSample(volume, ray.org + ray.t * ray.dir);

    if(isovalue >= min(sample0, sample) && isovalue <= max(sample0, sample)) {

      //! Find the t value for the isosurface intersection.
      const float tHit = t0 + (isovalue - sample0) / (sample - sample0) * (t - t0);

      //! Isosurface hit point.
      const vec3f coordinates = ray.org + tHit * ray.dir;

      //! Compute lighting.
      vec3f lightDirection;
      float lightDistance;
      vec3f lightRadiance = renderer->lights[0]->computeRadiance(renderer->lights[0], coordinates, lightDirection, lightDistance);

      //! Constant ambient lighting term.
      const float ambient = 0.1f;

      //! Use volume gradient as the normal.
      const vec3f gradient = normalize(volume->computeGradient(volume, coordinates));

      const float cosNL = isnan(gradient.x+gradient.y+gradient.z) ? 1.f : abs(dot(normalize(lightDirection), normalize(gradient)));

      //! Look up the color associated with the isovalue.
      vec3f sampleColor = volume->transferFunction->getColorForValue(volume->transferFunction, isovalue);

      sampleColor = sampleColor * (ambient + cosNL*(1.f-ambient)) * lightRadiance;

      //! Assume fully opaque isosurfaces for now.
      const float sampleOpacity = 1.f;

      //! Set the color contribution for this sample only (do not accumulate).
      color = sampleOpacity * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);

      ray.t = tHit;

      //! Only one hit at a time.
      return;
    }

    t0 = t;
    sample0 = sample;
  }
}

inline void RaycastVolumeRenderer_computeGeometrySample(RaycastVolumeRenderer *uniform renderer,
                                                        varying Ray &ray,
                                                        varying vec4f &color)
{
  //! We compute intersections on both the model and dynamicModel, and provide the contribution for the closest hit.
  bool hitDynamicModel = false;

  Ray dynamicRay = ray;
  traceRay(renderer->dynamicModel, dynamicRay);

  traceRay(renderer->model, ray);

  if(dynamicRay.t < ray.t) {
    hitDynamicModel = true;
    ray = dynamicRay;
  }

  //! No hit found.
  if(ray.geomID < 0) {
    ray.t = infinity;
    return;
  }

  //! Post intersect on the hit geometry.
  DifferentialGeometry dg;

  if(hitDynamicModel)
    postIntersect(renderer->dynamicModel, dg, ray, DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_MATERIALID|DG_COLOR|DG_TEXCOORD);
  else
    postIntersect(renderer->model, dg, ray, DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_MATERIALID|DG_COLOR|DG_TEXCOORD);

  //! Color of the geometry.
  vec3f sampleColor = make_vec3f(dg.color.x, dg.color.y, dg.color.z);

  //! Default opacity of 1.
  float sampleOpacity = 1.0f;

  //! If no color found in the geometry, map through transfer function.
  if(sampleColor.x < 0.f) {

    //! Coordinates of the geometry hit.
    const vec3f geometryCoordinates = ray.org + ray.t * ray.dir;

    //! Sample the volume.
    const float sample = renderer->model->volumes[0]->computeSample(renderer->model->volumes[0], geometryCoordinates);

    //! Look up the color associated with the volume sample. NaN values will have a color of (0,0,0) and a 0 opacity.
    sampleColor = isnan(sample) ? sampleOpacity = 0.f, make_vec3f(0.f) : renderer->model->volumes[0]->transferFunction->getColorForValue(renderer->model->volumes[0]->transferFunction, sample);
  }

  //! Compute lighting.
  vec3f lightDirection;
  float lightDistance;
  vec3f lightRadiance = renderer->lights[0]->computeRadiance(renderer->lights[0], dg.P, lightDirection, lightDistance);

  //! Constant ambient lighting term.
  const float ambient = 0.5f;

  const float cosNL = abs(dot(normalize(lightDirection), normalize(dg.Ns)));

  sampleColor = sampleColor * (ambient + cosNL*(1.f-ambient)) * lightRadiance;

  //! Set the color contribution for this sample only (do not accumulate).
  color = sampleOpacity * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);

}

inline void RaycastVolumeRenderer_intersectBox(const uniform box3f &box, 
                                               varying Ray &ray) 
{
  //! Intersection interval minimum per axis.
  const vec3f minimum = rcp(ray.dir) * (box.lower - ray.org);

  //! Intersection interval maximum per axis.
  const vec3f maximum = rcp(ray.dir) * (box.upper - ray.org);

  //! Distance along the ray to the entry point.
  ray.t0 = max(max(ray.t0, min(minimum.x, maximum.x)), max(min(minimum.y, maximum.y), min(minimum.z, maximum.z)));

  //! Distance along the ray to the exit point.
  ray.t1 = min(min(ray.t, max(minimum.x, maximum.x)), min(max(minimum.y, maximum.y), max(minimum.z, maximum.z)));

}

//! This function intersects the volume and geometries.
inline void RaycastVolumeRenderer_intersect(uniform RaycastVolumeRenderer *uniform renderer,
                                            varying Ray &ray,
                                            const varying float &rayOffset,
                                            varying vec4f &color)
{
  //! Assume just one volume.

  //! Box representing the global bounds of the volume.
  const uniform box3f bounds = make_box3f(make_vec3f(0.0f), make_vec3f(1.0f));

  //! Compute the intersection interval over the ray and volume bounds.
  RaycastVolumeRenderer_intersectBox(bounds, ray);

  if (ray.t0 > ray.t1)
    return;

  ray.t = ray.t0;

  //! Offset ray by a fraction of the nominal ray step.
  const uniform float step = 1.f / reduce_max(renderer->model->volumes[0]->upperBound) / renderer->model->volumes[0]->samplingRate;
  ray.t += rayOffset * step;

  //! Copy of the ray for volume isosurface intersection. The original ray is used for volume intersection.
  Ray isosurfaceRay = ray;
  isosurfaceRay.t = ray.t0 + rayOffset * 1.f / reduce_max(renderer->model->volumes[0]->upperBound); // don't consider sampling rate, but still allow offset.
  isosurfaceRay.primID = -1;
  isosurfaceRay.geomID = -1;
  isosurfaceRay.instID = -1;

  //! Copy of the ray for geometry intersection.
  Ray geometryRay = isosurfaceRay;
  geometryRay.t = ray.t1; //! end of valid ray interval for traceRay().

  //! Separate color contributions for the volume, isosurfaces, and geometries. Initialize to provided color in case of no contribution...
  vec4f volumeColor = color;
  vec4f isosurfaceColor = color;
  vec4f geometryColor = color;

  //! Initial trace through the volume and geometries.
  RaycastVolumeRenderer_computeVolumeSample(renderer, renderer->model->volumes[0], ray, volumeColor);
  RaycastVolumeRenderer_computeIsosurfaceSample(renderer, renderer->model->volumes[0], isosurfaceRay, isosurfaceColor);
  RaycastVolumeRenderer_computeGeometrySample(renderer, geometryRay, geometryColor);

  //! Trace the ray through the volume and geometries.
  float firstHit;

  while ((firstHit = min(min(ray.t, isosurfaceRay.t), geometryRay.t)) < ray.t1 && min(min(color.x, color.y), color.z) < 1.0f && color.w < 0.99f) {

    if (firstHit == ray.t) {

      //! Volume contribution.
      color = color + (1.0f - color.w) * volumeColor;

      //! Trace next volume ray.
      RaycastVolumeRenderer_computeVolumeSample(renderer, renderer->model->volumes[0], ray, volumeColor);
    }
    else if (firstHit == isosurfaceRay.t) {

      //! Isosurface contribution.
      color = color + (1.0f - color.w) * isosurfaceColor;

      //! Reset isosurface ray.
      isosurfaceRay.t = isosurfaceRay.t + 1e-4f;
      isosurfaceRay.primID = -1;
      isosurfaceRay.geomID = -1;
      isosurfaceRay.instID = -1;

      //! Trace next isosurface ray.
      RaycastVolumeRenderer_computeIsosurfaceSample(renderer, renderer->model->volumes[0], isosurfaceRay, isosurfaceColor);
    }
    else if (firstHit == geometryRay.t) {

      //! Geometry contribution.
      color = color + (1.0f - color.w) * geometryColor;

      //! Reset geometry ray.
      geometryRay.t0 = geometryRay.t + 1e-4f;
      geometryRay.t = ray.t1; //! end of valid ray interval for traceRay()
      geometryRay.primID = -1;
      geometryRay.geomID = -1;
      geometryRay.instID = -1;

      //! Trace next geometry ray.
      RaycastVolumeRenderer_computeGeometrySample(renderer, geometryRay, geometryColor);
    }
  }
}

void RaycastVolumeRenderer_renderSample(Renderer *uniform pointer, 
                                        varying ScreenSample &sample) 
{
  //! Cast to the actual Renderer subtype.
  RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

  //! Background color.
  const vec4f background = make_vec4f(1.0f);

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

  //! Gamma correction.
  color = renderer->model->volumes[0]->gammaCorrection.x * pow(color, renderer->model->volumes[0]->gammaCorrection.y);

  //! Attenuate the foreground and background colors by the opacity.
  color = color.w * color + (1.0f - color.w) * background;

  //! Store the result in the sample.
  sample.rgb.x = color.x;  sample.rgb.y = color.y;  sample.rgb.z = color.z;  sample.alpha = color.w;

}

export void *uniform RaycastVolumeRenderer_createInstance() {

  //! The renderer object.
  RaycastVolumeRenderer *uniform renderer = uniform new uniform RaycastVolumeRenderer;

  //! Constructor of the parent class.
  Renderer_Constructor(&renderer->inherited, NULL);

  //! Function to compute the color and opacity for a screen space sample.
  renderer->inherited.renderSample = RaycastVolumeRenderer_renderSample;

  //! Function to perform per-frame state initialization.
  renderer->inherited.beginFrame = RaycastVolumeRenderer_renderFramePreamble;

  //! Function to perform per-frame state completion.
  renderer->inherited.endFrame = RaycastVolumeRenderer_renderFramePostamble;  return(renderer);

}

export void RaycastVolumeRenderer_setCamera(void *uniform pointer, 
                                            void *uniform camera) 
{
  //! Cast to the actual Renderer subtype.
  RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

  //! Set the camera view to render.
  renderer->inherited.camera = (Camera *uniform) camera;

}

export void RaycastVolumeRenderer_setLights(void *uniform pointer, 
                                            void **uniform lights) 
{

  //! Cast to the actual Renderer subtype.
  RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

  //! Set the light sources.
  renderer->lights = (Light **uniform) lights;

}

export void RaycastVolumeRenderer_setModel(void *uniform pointer, 
                                           void *uniform model) 
{
  //! Cast to the actual Renderer subtype.
  RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

  //! Set the model to be rendered.
  renderer->model = (Model *uniform) model;

}

export void RaycastVolumeRenderer_setDynamicModel(void *uniform pointer, 
                                                  void *uniform model) 
{
  //! Cast to the actual Renderer subtype.
  RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

  //! Set the model to be rendered.
  renderer->dynamicModel = (Model *uniform) model;

}
