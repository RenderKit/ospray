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
#include "SimpleAO.ih"
#include "surfaces.ih"
#include "volumes.ih"

void SimpleAO_renderSample(Renderer *uniform _self,
                           FrameBuffer *uniform fb,
                           World *uniform world,
                           void *uniform perFrameData,
                           varying ScreenSample &sample)
{
  uniform SimpleAO *uniform self = (uniform SimpleAO * uniform) _self;

  vec4f outputColor = make_vec4f(0.f);

  uniform bool firstHit       = true;
  const float originalRayTFar = sample.ray.t;

  while (outputColor.w < 0.99f) {
    Ray &geomRay  = sample.ray;
    Ray volumeRay = sample.ray;

    traceRay(world, geomRay);

    VolumeInterval volumeInterval;
    traceVolumeRay(world, volumeRay, volumeInterval);

    const bool haveGeometryHit = hadHit(geomRay);
    const bool haveVolumeHit   = volumeInterval.hasInterval;

    const bool bothHit   = haveGeometryHit && haveVolumeHit;
    const bool eitherHit = haveGeometryHit || haveVolumeHit;

    const bool volumeFirst = volumeInterval.interval.x < geomRay.t;

    cif(firstHit)
    {
      sample.z = geomRay.t;
      if (!(eitherHit)) {
        sample.rgb    = make_vec3f(self->super.bgColor);
        sample.albedo = sample.rgb;
        sample.normal = sample.ray.dir;
        sample.alpha  = self->super.bgColor.w;
      }
    }

    if (!(eitherHit))
      break;

    vec4f volumeColor  = make_vec4f(0.f);
    vec4f surfaceColor = make_vec4f(0.f);

    DifferentialGeometry dg;
    if (haveGeometryHit) {
      computeDG(world, geomRay, dg);
      surfaceColor =
          computeShadedSurfaceColor(self, fb, world, firstHit, dg, sample);
    }

    if (haveVolumeHit && volumeFirst) {
      volumeInterval.interval.y = min(geomRay.t, volumeInterval.interval.y);
      setRay(volumeRay, volumeInterval.interval.x, volumeInterval.interval.y);
      volumeColor = integrateVolumeInterval(sample, volumeInterval, volumeRay);
    }

    vec4f blendedColor = make_vec4f(0.f);

    if (bothHit) {
      blendedColor = volumeColor + (1.0f - volumeColor.w) * surfaceColor;
    } else {
      blendedColor = haveGeometryHit ? surfaceColor : volumeColor;
    }

    outputColor = outputColor + (1.0f - outputColor.w) * blendedColor;

    setRay(geomRay, geomRay.t + dg.epsilon, originalRayTFar);

    firstHit = false;
  }

  sample.rgb   = make_vec3f(outputColor);
  sample.alpha = outputColor.w;
}

// SimpleAO C++ interface /////////////////////////////////////////////////////

export void *uniform SimpleAO_create(void *uniform cppE)
{
  uniform SimpleAO *uniform self = uniform new uniform SimpleAO;
  Renderer_Constructor(&self->super, cppE, 1);
  self->super.renderSample = SimpleAO_renderSample;
  return self;
}

export void SimpleAO_set(void *uniform _self,
                         uniform int samplesPerFrame,
                         uniform float aoRadius,
                         uniform float aoIntensity)
{
  uniform SimpleAO *uniform self = (uniform SimpleAO * uniform) _self;

  self->samplesPerFrame = samplesPerFrame;
  self->aoRadius        = aoRadius;
  self->aoIntensity     = aoIntensity;
}
