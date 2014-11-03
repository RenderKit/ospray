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

inline void RaycastVolumeRenderer_computeSample(Volume *uniform volume, varying Ray &ray, varying vec4f &color) {

    //! Advance the ray.
    volume->intersect(volume, ray);  if (ray.t > ray.t1) return;

    //! Sample the volume at the hit point in world coordinates.
    const float sample = volume->computeSample(volume, ray.org + ray.t * ray.dir);

    //! Look up the color associated with the volume sample.
    const vec3f sampleColor = volume->transferFunction->getColorForValue(volume->transferFunction, sample);

    //! Look up the opacity associated with the volume sample.
    const float sampleAlpha = volume->transferFunction->getAlphaForValue(volume->transferFunction, sample);

    //! Attenuate the sampled opacity by step count so the apparent intensity remains constant.
    const float transparency = (1.0f - color.w) * (1.0f - exp(-sampleAlpha * volume->stepSize));

    //! Attenuate the sampled color by the remaining transparency.
    color = color + transparency * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);

}

inline void RaycastVolumeRenderer_intersectBox(const uniform box3f &box, varying Ray &ray) {

    //! Intersection interval minimum per axis.
    const vec3f minimum = rcp(ray.dir) * (box.lower - ray.org);

    //! Intersection interval maximum per axis.
    const vec3f maximum = rcp(ray.dir) * (box.upper - ray.org);

    //! Distance along the ray to the entry point.
    ray.t0 = max(max(ray.t0, min(minimum.x, maximum.x)), max(min(minimum.y, maximum.y), min(minimum.z, maximum.z)));

    //! Distance along the ray to the exit point.
    ray.t1 = min(min(ray.t, max(minimum.x, maximum.x)), min(max(minimum.y, maximum.y), max(minimum.z, maximum.z)));

}

inline void RaycastVolumeRenderer_intersectVolume(Volume *uniform volume, varying Ray &ray, varying vec4f &color) {

    //! Box representing the global bounds of the volume.
    const uniform box3f bounds = make_box3f(make_vec3f(0.0f), make_vec3f(1.0f));

    //! Compute the intersection interval over the ray and volume bounds.
    RaycastVolumeRenderer_intersectBox(bounds, ray);  if (ray.t0 > ray.t1) return;

    //! Adjust the ray so the initial hit point will lie just inside the volume.
    ray.t = ray.t0 - volume->stepSize * 0.5f;
 
    //! Trace the ray through the volume.
    while (ray.t < ray.t1 && reduce_min(color) < 1.0f) RaycastVolumeRenderer_computeSample(volume, ray, color);

}

void RaycastVolumeRenderer_renderSample(Renderer *uniform pointer, varying ScreenSample &sample) {

    //! Cast to the actual Renderer subtype.
    RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

    //! Background color.
    const vec4f background = make_vec4f(1.0f);

    //! For now we assume the model contains at least 1 volume.
    vec4f color = make_vec4f(0.0f);  RaycastVolumeRenderer_intersectVolume(renderer->model->volumes[0], sample.ray, color);

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
    renderer->inherited.renderSample = RaycastVolumeRenderer_renderSample;

    //! Function to perform per-frame state initialization.
    renderer->inherited.beginFrame = RaycastVolumeRenderer_renderFramePreamble;

    //! Function to perform per-frame state completion.
    renderer->inherited.endFrame = RaycastVolumeRenderer_renderFramePostamble;  return(renderer);

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

