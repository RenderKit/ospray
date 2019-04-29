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
#include "surfaceShading.ih"

void SimpleAO_renderSample(Renderer *uniform _self,
                           FrameBuffer *uniform fb,
                           World *uniform world,
                           void *uniform perFrameData,
                           varying ScreenSample &sample)
{
  uniform SimpleAO *uniform self = (uniform SimpleAO * uniform) _self;

  traceRay(world, sample.ray);
  sample.z = sample.ray.t;

  if (noHit(sample.ray)) {
    sample.rgb   = make_vec3f(self->super.bgColor);
    sample.alpha = self->super.bgColor.w;
    return;
  }

  const vec4f color = computeShadedSurfaceColor(self, fb, world, sample);

  sample.rgb   = make_vec3f(color);
  sample.alpha = color.w;
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
                         uniform float aoRadius)
{
  uniform SimpleAO *uniform self = (uniform SimpleAO * uniform) _self;

  self->samplesPerFrame = samplesPerFrame;
  self->aoRadius        = aoRadius;
}
