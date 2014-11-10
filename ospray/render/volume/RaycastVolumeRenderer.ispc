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

inline void RaycastVolumeRenderer_computeVolumeSample(Volume *uniform volume, varying Ray &ray, varying vec4f &color) {

    //! Advance the ray.
    volume->intersect(volume, ray);  if (ray.t > ray.t1) return;

    //! Sample the volume at the hit point in world coordinates.
    const float sample = volume->computeSample(volume, ray.org + ray.t * ray.dir);

    //! Look up the color associated with the volume sample.
    const vec3f sampleColor = volume->transferFunction->getColorForValue(volume->transferFunction, sample);

    //! Look up the opacity associated with the volume sample.
    const float sampleOpacity = volume->transferFunction->getOpacityForValue(volume->transferFunction, sample);

    //! Set the color contribution for this sample only (do not accumulate).
//  color = (1.0f - exp(-sampleOpacity * volume->stepSize)) * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);
    color = clamp(sampleOpacity / volume->samplingRate) * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);

}

inline void RaycastVolumeRenderer_computeGeometrySample(RaycastVolumeRenderer *uniform renderer, varying Ray &ray, varying vec4f &color) {

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

    //! If no color found in the geometry, map through transfer function.
    if(sampleColor.x == -1.f) {

        //! Coordinates of the geometry hit.
        const vec3f geometryCoordinates = ray.org + ray.t * ray.dir;

        //! Sample the volume.
        const float sample = renderer->model->volumes[0]->computeSample(renderer->model->volumes[0], geometryCoordinates);

        //! Look up the color associated with the volume sample.
        sampleColor = renderer->model->volumes[0]->transferFunction->getColorForValue(renderer->model->volumes[0]->transferFunction, sample);
    }
    else {

        //! Lighting. For now only on geometry with color information.
        vec3f lightDirection;
        float lightDistance;
        vec3f lightRadiance = renderer->lights[0]->computeRadiance(renderer->lights[0], dg.P, lightDirection, lightDistance);

        const float cosNL = abs(dot(normalize(lightDirection), normalize(dg.Ns)));

        sampleColor = sampleColor * cosNL * lightRadiance;
    }

    //! Assume constant opacity for now.
    const float sampleOpacity = 1.0f;

    //! Set the color contribution for this sample only (do not accumulate).
    color = sampleOpacity * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);

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

//! This function intersects the volume and geometries.
inline void RaycastVolumeRenderer_intersect(RaycastVolumeRenderer *uniform renderer, varying Ray &ray, varying vec4f &color) {

    //! Assume just one volume.

    //! Box representing the global bounds of the volume.
    const uniform box3f bounds = make_box3f(make_vec3f(0.0f), make_vec3f(1.0f));

    //! Compute the intersection interval over the ray and volume bounds.
    RaycastVolumeRenderer_intersectBox(bounds, ray);  if (ray.t0 > ray.t1) return;  ray.t = ray.t0;

    //! Copy of the ray for geometry intersection. The original ray is used for volume intersection.
    Ray geometryRay = ray;
    geometryRay.t = ray.t1; //! end of valid ray interval for traceRay()
    geometryRay.primID = -1;
    geometryRay.geomID = -1;
    geometryRay.instID = -1;

    //! Separate color contributions for the volume and geometries. Initialize to provided color in case of no volume contribution...
    vec4f volumeColor = color;
    vec4f geometryColor = color;

    //! Initial trace through the volume and geometries.
    RaycastVolumeRenderer_computeVolumeSample(renderer->model->volumes[0], ray, volumeColor);
    RaycastVolumeRenderer_computeGeometrySample(renderer, geometryRay, geometryColor);

renderer->screenSamples++;

    //! Trace the ray through the volume and geometries.
    while (min(ray.t, geometryRay.t) < ray.t1 && min(min(color.x, color.y), color.z) < 1.0f && color.w < 0.99f) {

        if (geometryRay.t < ray.t) {

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

        } else {

            //! Volume contribution.
            color = color + (1.0f - color.w) * volumeColor;

            //! Trace next volume ray.
            RaycastVolumeRenderer_computeVolumeSample(renderer->model->volumes[0], ray, volumeColor);

renderer->volumeSamples++;

        }

    }

}

void RaycastVolumeRenderer_renderSample(Renderer *uniform pointer, varying ScreenSample &sample) {

    //! Cast to the actual Renderer subtype.
    RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

    //! Background color.
    const vec4f background = make_vec4f(1.0f);

    //! Provide the renderer to the intersector as it contains all volumes, geometries, etc.
    vec4f color = make_vec4f(0.0f);  RaycastVolumeRenderer_intersect(renderer, sample.ray, color);

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

export void RaycastVolumeRenderer_setDynamicModel(void *uniform pointer, void *uniform model) {

    //! Cast to the actual Renderer subtype.
    RaycastVolumeRenderer *uniform renderer = (RaycastVolumeRenderer *uniform) pointer;

    //! Set the model to be rendered.
    renderer->dynamicModel = (Model *uniform) model;

}
