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

#include "Material.ih"
#include "render/pathtracer/bsdfs/Transmission.ih"

struct Luminous
{
  PathTraceMaterial super;
  float transparency;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* varying 
Luminous_getBSDF(const uniform PathTraceMaterial* uniform _self
    , uniform ShadingContext* uniform ctx
    , const DifferentialGeometry& dg
    , const Ray&
    , const Medium&
    )
{ 
  uniform Luminous* uniform self = (uniform Luminous* uniform)_self;
  varying BSDF* uniform bsdf = NULL;

  if (self->transparency > 0.0f) {
    vec3f T = make_vec3f(self->transparency);
    bsdf = Transmission_create(ctx, NULL/*no frame needed*/, T);
  }

  return bsdf;
}

vec3f Luminous_getTransparency(const uniform PathTraceMaterial* uniform _self
    , const DifferentialGeometry&
    , const Ray&
    , const Medium&
    )
{
  uniform Luminous* uniform self = (uniform Luminous* uniform)_self;

  return make_vec3f(self->transparency);
}

export void PathTracer_Luminous_set(void* uniform
    , const uniform vec3f&
    , const uniform float
    );

void Luminous_Constructor(uniform Luminous* uniform self)
{
  PathTraceMaterial_Constructor(&self->super
      , Luminous_getBSDF
      , Luminous_getTransparency
      );
  PathTracer_Luminous_set(self, make_vec3f(1.f), 0.f);
}


///////////////////////////////////////////////////////////////////////////////
// External API

export void PathTracer_Luminous_set(void* uniform _self
    , const uniform vec3f& radiance
    , const uniform float transparency
    )
{
  uniform Luminous* uniform self = (uniform Luminous* uniform)_self;
  self->super.emission = radiance;
  self->transparency = transparency;
}

export void* uniform PathTracer_Luminous_create()
{
  uniform Luminous* uniform self = uniform new uniform Luminous;
  Luminous_Constructor(self);
  return self;
}
