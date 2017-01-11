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
#include "../bsdfs/ThinDielectric.ih"

struct ThinGlass
{
  PathTraceMaterial super;

  float eta;
  vec3f transmission;
  float thickness;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* uniform ThinGlass_getBSDF(const uniform PathTraceMaterial* uniform super,
                                              uniform ShadingContext* uniform ctx,
                                              const DifferentialGeometry& dg,
                                              const Ray& ray,
                                              const Medium& currentMedium)
{
  const uniform ThinGlass* uniform self = (const uniform ThinGlass* uniform)super;

  return ThinDielectric_create(ctx, LinearSpace3f_create(ctx, frame(dg.Ns)), self->eta, self->transmission, self->thickness);
}

vec3f ThinGlass_getTransparency(const uniform PathTraceMaterial* uniform material,
                                const DifferentialGeometry& dg,
                                const Ray& ray,
                                const Medium& currentMedium)
{
  const uniform ThinGlass* uniform self = (const uniform ThinGlass* uniform)material;

  float cosThetaO = max(-dot(ray.dir, dg.Ns), 0.0f);
  float alpha = self->thickness * rcp(cosThetaO);
  return powf(self->transmission, alpha) * (1.0f-fresnelDielectric(cosThetaO, self->eta));
}

void ThinGlass_Constructor(uniform ThinGlass* uniform self,
                           uniform float ior,
                           const uniform vec3f& transmission,
                           uniform float thickness)
{
  PathTraceMaterial_Constructor(&self->super, ThinGlass_getBSDF, ThinGlass_getTransparency);
  self->eta = rcp(ior);
  self->transmission = transmission;
  self->thickness = thickness;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_ThinGlass_create(uniform float ior,
                                                 const uniform vec3f& transmission,
                                                 uniform float thickness)
{
  uniform ThinGlass* uniform self = uniform new uniform ThinGlass;
  ThinGlass_Constructor(self, ior, transmission, thickness);
  return self;
}
