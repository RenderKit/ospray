// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "render/Renderer.ih"
// ao renderer
#include "SciVis.ih"
#include "surfaces.ih"
#include "volumes.ih"
#include "math/random.ih"

void SciVis_renderSample(Renderer *uniform _self,
                         FrameBuffer *uniform fb,
                         World *uniform world,
                         void *uniform perFrameData,
                         varying ScreenSample &sample)
{
  uniform SciVis *uniform self = (uniform SciVis * uniform) _self;

  LDSampler ldSamplerObj;
  varying LDSampler* uniform ldSampler = &ldSamplerObj;
  LDSampler_init(ldSampler, sample.sampleID.x + fb->size.x * sample.sampleID.y, sample.sampleID.z);

  const uniform vec4f &bgColor = self->super.bgColor;

  vec4f outputColor = make_vec4f(0.f);

  uniform bool firstHit       = true;
  const float originalRayTFar = sample.ray.t;

  // This is our main ray
  Ray &ray = sample.ray;

  // Iterate over all translucent geometry till we are fully opaque
  while (outputColor.w < 0.99f) {

    // Trace geometry using given ray,
    // if hit ray.t will be updated
    traceRay(world, ray);

    // Determine volume intervals by tracing ray in the volume scene
    Ray volumeRay = ray;
    VolumeIntervals volumeIntervals;
    traceVolumeRay(world, volumeRay, volumeIntervals);

    // Sample volumes across volume intervals (in front of geometry hit)
    if (volumeIntervals.numVolumeIntervals > 0) {
      vec4f volumeColor = integrateVolumeIntervals(
        volumeIntervals, volumeRay, ldSampler);
      outputColor = outputColor + (1.f - outputColor.w) * volumeColor;
    }

    // If any geometry has been hit
    vec4f blendedColor;
    if (hadHit(ray)) {

      // Prepare differential geometry structure
      DifferentialGeometry dg;
      computeDG(world, ray, dg);

      // Shade geometry
      SSI surfaceShading;
      surfaceShading = computeShading(self, fb, world, dg, sample);

      // Use shaded color for blending
      blendedColor = surfaceShading.shadedColor;

      // Prepare ray for next loop iteration,
      // start from the last geometry hit all over to initial Tfar
      setRay(ray, ray.t + dg.epsilon, originalRayTFar);

      // Initialize other per sample data with first hit values
      cif (firstHit) {
        sample.z      = ray.t;
        sample.albedo = surfaceShading.albedo;
        sample.normal = dg.Ns;
      }
    } else {

      // Use background color if no geometry has been hit
      blendedColor = bgColor;
      blendedColor.w = 1.f;

      // Initialize other per sample data with first hit values
      cif (firstHit) {
        sample.z      = inf;
        sample.albedo = make_vec3f(bgColor);
        sample.normal = sample.ray.dir;
      }
    }

    // Blend with output final color
    outputColor = outputColor + (1.f - outputColor.w) * blendedColor;
    firstHit = false;
  }

  sample.rgb   = make_vec3f(outputColor);
  sample.alpha = outputColor.w;
}

// SciVis C++ interface /////////////////////////////////////////////////////

export void *uniform SciVis_create(void *uniform cppE)
{
  uniform SciVis *uniform self = uniform new uniform SciVis;
  Renderer_Constructor(&self->super, cppE, 1);
  self->super.renderSample = SciVis_renderSample;
  return self;
}

export void SciVis_set(void *uniform _self,
                       uniform int aoSamples,
                       uniform float aoRadius,
                       uniform float aoIntensity)
{
  uniform SciVis *uniform self = (uniform SciVis * uniform) _self;

  self->aoSamples   = aoSamples;
  self->aoRadius    = aoRadius;
  self->aoIntensity = aoIntensity;
}
