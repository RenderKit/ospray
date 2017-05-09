// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

//ospray
#include "common/Model.ih"
#include "render/Renderer.ih"

struct DistributedRaycastRenderer
{
  uniform Renderer super;
};

void DistributedRaycastRenderer_renderSample(uniform Renderer *uniform _self,
                                             void *uniform perFrameData,
                                             varying ScreenSample &sample)
{
  uniform DistributedRaycastRenderer *uniform self =
    (uniform DistributedRaycastRenderer *uniform)_self;

  traceRay(self->super.model, sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  if (sample.ray.geomID < 0)
    sample.rgb = make_vec3f(self->super.bgColor);
  else {
    DifferentialGeometry dg;
    dg.color = make_vec4f(0.f);
    postIntersect(self->super.model, dg, sample.ray,
                  DG_COLOR | DG_MATERIALID | DG_NG | DG_NS);
    sample.rgb = make_vec3f(dg.color) *
                 abs(dot(normalize(sample.ray.dir), normalize(dg.Ns)));
  }
}

// Exported functions /////////////////////////////////////////////////////////

export void *uniform DistributedRaycastRenderer_create(void *uniform cppE)
{
  uniform DistributedRaycastRenderer *uniform self =
    uniform new uniform DistributedRaycastRenderer;

  Renderer_Constructor(&self->super, cppE, NULL, NULL, 1);
  self->super.renderSample = DistributedRaycastRenderer_renderSample;

  return self;
}
