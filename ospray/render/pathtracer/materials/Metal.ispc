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

#include "Material.ih"
#include "texture/TextureParam.ih"
#include "../bsdfs/Conductor.ih"
#include "../bsdfs/MicrofacetConductor.ih"

struct Metal
{
  PathTraceMaterial super;

  vec3f eta; // index of refraction (wavelength dependent -> RGB)
  vec3f k; // index of refraction, imaginary part (wavelength dependent -> RGB)
  float roughness; // in [0, 1]; 0==ideally smooth (mirror)
  TextureParam map_roughness;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* uniform Metal_getBSDF(const uniform PathTraceMaterial* uniform super,
                                          uniform ShadingContext* uniform ctx,
                                          const DifferentialGeometry& dg,
                                          const Ray& ray,
                                          const Medium& currentMedium)
{
  const uniform Metal* uniform self = (const uniform Metal* uniform)super;
  varying linear3f* uniform frame = LinearSpace3f_create(ctx, frame(dg.Ns));

  float roughness = self->roughness;
  if (valid(self->map_roughness))
    roughness *= get1f(self->map_roughness, dg.st);

  if (roughness == 0.0f)
    return Conductor_create(ctx, frame, self->eta, self->k);
  else
    return MicrofacetConductor_create(ctx, frame, self->eta, self->k, roughness);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Metal_set(void* uniform _self
    , const uniform vec3f& eta
    , const uniform vec3f& k
    , uniform float roughness
    , void* uniform map_roughness
    , const uniform affine2f &xform_roughness
    )
{
  Metal* uniform self = (Metal* uniform)_self;
  self->eta = eta;
  self->k = k;
  self->roughness = roughness;
  self->map_roughness =
    make_TextureParam((uniform Texture2D*)map_roughness,
        xform_roughness);
}

export void* uniform PathTracer_Metal_create()
{
  Metal* uniform self = uniform new Metal;
  PathTraceMaterial_Constructor(&self->super, Metal_getBSDF);
  PathTracer_Metal_set(self, make_vec3f(1.f), make_vec3f(1.f), 0.1f, 
      NULL, make_AffineSpace2f_identity());
  return self;
}

