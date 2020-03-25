// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Renderer.ih"
// ao renderer
#include "SciVis.ih"
#include "math/random.ih"
#include "surfaces.ih"
#include "volumes.ih"

void SciVis_renderSample(Renderer *uniform _self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  uniform SciVis *uniform self = (uniform SciVis * uniform) _self;

  LDSampler ldSamplerObj;
  varying LDSampler *uniform ldSampler = &ldSamplerObj;
  LDSampler_init(ldSampler,
      sample.sampleID.x + fb->size.x * sample.sampleID.y,
      sample.sampleID.z);

  uniform bool firstHit = true;
  const float originalRayTFar = sample.ray.t;

  // This is our main ray
  Ray &ray = sample.ray;

  // First trace the ray across clipping scene to calculate ray intervals,
  // this step should keep ray structure unchanged
  RayIntervals rayIntervals;
  traceClippingRay(world, ray, rayIntervals);

  // Iterate over all translucent geometry till we are fully opaque
  vec4f outputColor = make_vec4f(0.f);
  while (outputColor.w < 0.99f) {
    // Then trace normal geometry using calculated ray intervals,
    // if hit ray.t will be updated
    traceGeometryRayIntervals(world, ray, rayIntervals);

    // Determine volume intervals by tracing ray in the volume scene
    Ray volumeRay = ray;
    VolumeIntervals volumeIntervals;
    traceVolumeRay(world, volumeRay, volumeIntervals);

    // Sample volumes across volume intervals (in front of geometry hit)
    if (volumeIntervals.numVolumeIntervals > 0) {
      vec4f volumeColor = integrateVolumeIntervals(volumeIntervals,
          rayIntervals,
          volumeRay,
          ldSampler,
          self->volumeSamplingRate);
      outputColor = outputColor + (1.f - outputColor.w) * volumeColor;
    }

    // If any geometry has been hit
    vec4f blendedColor;
    const bool hadHit = hadHit(ray);
    if (hadHit) {
      // Prepare differential geometry structure
      DifferentialGeometry dg;
      computeDG(world, self, ray, dg);

      // Shade geometry
      SSI surfaceShading;
      surfaceShading = computeShading(self, fb, world, dg, sample);

      // Use shaded color for blending
      blendedColor = surfaceShading.shadedColor;

      // Prepare ray for next loop iteration,
      // start from the last geometry hit all over to initial Tfar
      setRay(ray, ray.t + dg.epsilon, originalRayTFar);

      // Initialize other per sample data with first hit values
      if (firstHit) {
        sample.z = ray.t;
        sample.albedo = surfaceShading.albedo;
        sample.normal = dg.Ns;
      }
    } else {
      blendedColor = Renderer_getBackground(&self->super, sample.pos);
      // Initialize other per sample data with first hit values
      if (firstHit) {
        sample.z = ray.t;
        sample.albedo = make_vec3f(blendedColor);
        sample.normal = sample.ray.dir;
      }
    }

    // Blend with output final color
    outputColor = outputColor + (1.f - outputColor.w) * blendedColor;
    firstHit = false;
    if (!hadHit) {
      break;
    }
  }

  sample.rgb = make_vec3f(outputColor);
  sample.alpha = outputColor.w;
}

// SciVis C++ interface /////////////////////////////////////////////////////

export void *uniform SciVis_create(void *uniform cppE)
{
  uniform SciVis *uniform self = uniform new uniform SciVis;
  Renderer_Constructor(&self->super, cppE);
  self->super.renderSample = SciVis_renderSample;
  return self;
}

export void SciVis_set(void *uniform _self,
    uniform int aoSamples,
    uniform float aoRadius,
    uniform float aoIntensity,
    uniform float volumeSamplingRate)
{
  uniform SciVis *uniform self = (uniform SciVis * uniform) _self;

  self->aoSamples = aoSamples;
  self->aoRadius = aoRadius;
  self->aoIntensity = aoIntensity;

  self->volumeSamplingRate = volumeSamplingRate;
}
