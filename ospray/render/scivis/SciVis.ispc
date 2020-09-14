// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Renderer.ih"
// ao renderer
#include "SciVis.ih"
#include "math/random.ih"
#include "math/sampling.ih"
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
      SciVisRenderContext rc;
      rc.renderer = self;
      rc.fb = fb;
      rc.world = world;
      rc.sample = sample;
      rc.ldSampler = ldSampler;
      vec4f volumeColor = integrateVolumeIntervalsGradient(rc,
          volumeIntervals,
          rayIntervals,
          volumeRay,
          ldSampler,
          self->volumeSamplingRate,
          true);

      outputColor = outputColor + (1.f - outputColor.w) * volumeColor;
    }

    // If any geometry has been hit
    vec4f blendedColor;
    const bool hadHit = hadHit(ray);
    if (hadHit) {
      // Prepare differential geometry structure
      DifferentialGeometry dg;
      postIntersect(world,
          &self->super,
          dg,
          ray,
          DG_NG | DG_NS | DG_NORMALIZE | DG_FACEFORWARD | DG_COLOR
              | DG_TEXCOORD);

      // Shade geometry
      SSI surfaceShading;
      surfaceShading = computeShading(
          self, fb, world, dg, sample, ldSampler, ray.dir, ray.time);

      // Use shaded color for blending
      blendedColor = surfaceShading.shadedColor;

      // Initialize other per sample data with first hit values
      if (firstHit) {
        sample.z = ray.t;
        sample.albedo = surfaceShading.albedo;
        sample.normal = dg.Ns;
      }

      // Prepare ray for next loop iteration,
      // start from the last geometry hit all over to initial Tfar
      setRay(ray, ray.t + dg.epsilon, originalRayTFar);
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
    uniform bool shadowsEnabled,
    uniform int aoSamples,
    uniform float aoRadius,
    uniform float volumeSamplingRate)
{
  uniform SciVis *uniform self = (uniform SciVis * uniform) _self;

  self->shadowsEnabled = shadowsEnabled;
  self->aoSamples = aoSamples;
  self->aoRadius = aoRadius;

  self->volumeSamplingRate = volumeSamplingRate;
}

float SciVis_computeAO(const uniform SciVis *uniform self,
    const FrameBuffer *uniform fb,
    const World *uniform world,
    ScreenSample &sample,
    varying LDSampler *uniform ldSampler,
    const varying DifferentialGeometry &dg,
    const uniform int sampleCnt,
    const uniform float aoRadius,
    const varying vec3i &sampleID)
{
  const uniform int accumID = reduce_max(sampleID.z) * sampleCnt;

  // init TEA RNG //
  RandomTEA rng_state;
  varying RandomTEA *const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, sampleID.x, sampleID.y);
  const vec2f rot = RandomTEA__getFloats(rng);

  float hits = 0.f;

  const linear3f localToWorld = frame(dg.Ns);

  for (uniform int i = 0; i < sampleCnt; i++) {
    // Use base-5 and base-7 Halton to avoid correlation with sample position
    const vec2f halton = make_vec2f(
        PermHalton_sample5(accumID + i), PermHalton_sample7(accumID + i));
    const vec2f r = CranleyPattersonRotation(halton, rot);
    const vec3f local_ao_dir = cosineSampleHemisphere(r);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    if (dot(ao_dir, dg.Ns) < 0.01f) { // check below surface
      hits += 1.f;
      continue;
    }

    Ray ao_ray;
    setRay(ao_ray, dg.P, ao_dir, dg.epsilon, aoRadius);

    hits = hits
        + (1.f
            - lightAlpha(self,
                ao_ray,
                fb,
                world,
                sample,
                ldSampler,
                1.f,
                dg.epsilon,
                0.1f));
  }

  // the cosTheta of cosineSampleHemispherePDF and dot(shadingNormal, ao_dir)
  // cancel
  return 1.0f - (hits / (float)sampleCnt);
}
