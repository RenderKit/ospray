//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "ospray/fb/framebuffer.ih"
#include "ospray/render/util.ih"
#include "ospray/render/volume/RaycastVolumeRenderer.ih"

inline void RaycastVolumeRenderer_computeSample(Volume *uniform volume, varying vec3f &coordinates, varying vec3f &increment, varying vec4f &color) {

    //! Sample the volume and advance the ray.
    const float sample = volume->computeSample(volume, coordinates);  coordinates = coordinates + increment;

    //! Look up the color associated with the volume sample.
    const vec3f sampleColor = volume->transferFunction->getColorForValue(volume->transferFunction, sample);

    //! Look up the opacity associated with the volume sample.
    const float sampleAlpha = volume->transferFunction->getAlphaForValue(volume->transferFunction, sample);

    //! Attenuate the sampled opacity by step count so the apparent intensity remains constant.
    const float transparency = (1.0f - color.w) * (1.0f - exp(-sampleAlpha * volume->stepSize));

    //! Attenuate the sampled color by the remaining transparency.
    color = color + transparency * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);

}

inline void RaycastVolumeRenderer_intersectBox(const varying Ray &ray, const uniform box3f &box, varying float &entry, varying float &exit) {

    //! Intersection interval minimum per axis.
    const vec3f minimum = rcp(ray.dir) * (box.lower - ray.org);

    //! Intersection interval maximum per axis.
    const vec3f maximum = rcp(ray.dir) * (box.upper - ray.org);

    //! Distance along the ray to the entry point.
    entry = max(max(ray.t0, min(minimum.x, maximum.x)), max(min(minimum.y, maximum.y), min(minimum.z, maximum.z)));

    //! Distance along the ray to the exit point.
    exit = min(min(ray.t, max(minimum.x, maximum.x)), min(max(minimum.y, maximum.y), max(minimum.z, maximum.z)));

}

inline void RaycastVolumeRenderer_intersectVolume(const varying Ray &ray, Volume *uniform volume, varying vec4f &color) {

    //! Box representing the global bounds of the volume.
    const uniform box3f bounds = make_box3f(make_vec3f(0.0f), make_vec3f(1.0f));

    //! Compute the intersection interval over the ray and volume bounds.
    float entry, exit;  RaycastVolumeRenderer_intersectBox(ray, bounds, entry, exit);  if (entry >= exit) return;
 
    //! The coordinates along the ray at which to sample the volume.
    vec3f coordinates = volume->upperBound * (ray.org + entry * ray.dir);

    //! Increment required to move the coordinates along the ray one step.
    vec3f increment = volume->upperBound * volume->stepSize * ray.dir;

    //! Trace the ray through the volume.
    for (float t = entry ; t < exit && reduce_min(color) < 1.0f ; t += volume->stepSize) RaycastVolumeRenderer_computeSample(volume, coordinates, increment, color);

}

void RaycastVolumeRenderer_renderSample(Renderer *uniform pointer, varying ScreenSample &sample) {

    //! Cast to the actual Renderer subtype.
    RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

    //! Background color.
    const vec4f background = make_vec4f(1.0f);

    //! For now we assume the model contains at least 1 volume.
    vec4f color = make_vec4f(0.0f);  RaycastVolumeRenderer_intersectVolume(sample.ray, renderer->model->volumes[0], color);

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
    Renderer_constructor(&renderer->inherited, NULL, NULL, NULL);

    //! Function to compute the color and opacity for a screen space sample.
    renderer->inherited.renderSample = RaycastVolumeRenderer_renderSample;  return(renderer);

}

export void RaycastVolumeRenderer_setCamera(void *uniform pointer, void *uniform camera) {

    //! Cast to the actual Renderer subtype.
    RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

    //! Set the camera view to render.
    renderer->inherited.camera = (Camera *uniform) camera;

}

export void RaycastVolumeRenderer_setLights(void *uniform pointer, void **uniform lights) {

    //! Cast to the actual Renderer subtype.
    RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

    //! Set the light sources.
    renderer->lights = (Light **uniform) lights;

}

export void RaycastVolumeRenderer_setModel(void *uniform pointer, void *uniform model) {

    //! Cast to the actual Renderer subtype.
    RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

    //! Set the model to be rendered.
    renderer->model = (Model *uniform) model;

}

