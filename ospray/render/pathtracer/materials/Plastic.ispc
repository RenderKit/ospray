// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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
#include "../bsdfs/DielectricLayerLambert.ih"
#include "../bsdfs/DielectricReflection.ih"
#include "../bsdfs/MicrofacetPlastic.ih"

struct Plastic
{
  PathTraceMaterial base;
  float roughness;
  DielectricLayerLambert substrate;
  DielectricReflection reflection;
  MicrofacetPlastic microfacet;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

void Plastic__shade(const uniform PathTraceMaterial *uniform _this,
                  const Ray&                  ray,
                  const Medium&               currentMedium,
                  const DifferentialGeometry& dg,
                  uniform CompositedBSDF&     brdfs,
                  vec3f& Ns)
{
  const uniform Plastic* uniform this = (const uniform Plastic* uniform)_this;
  CompositedBSDF_add(&brdfs,&this->substrate.base);
  if (this->roughness == 0.0f)  CompositedBSDF_add(&brdfs,&this->reflection.base);
  else                          CompositedBSDF_add(&brdfs,&this->microfacet.base);
}

void Plastic__Constructor(uniform Plastic *uniform this,
                          const uniform vec3f& pigmentColor,
                          const uniform float eta,
                          const uniform float roughness)
{
  PathTraceMaterial__Constructor(&this->base, Plastic__shade, NULL);
  this->roughness = roughness;

  DielectricLayerLambert_Constructor(&this->substrate,
                                         make_vec3f(1.0f), 1.0f, eta,
                                         make_Lambert(pigmentColor));

  DielectricReflection__Constructor(&this->reflection, 1.0f, eta);

  MicrofacetPlastic__Constructor(&this->microfacet,make_vec3f(1.0f),
                                 make_FresnelDielectric(1.0f,eta),
                                 make_PowerCosineDistribution(rcp(roughness)));
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Plastic_create(const uniform vec3f& pigmentColor,
                                  const uniform float eta,
                                  const uniform float roughness)
{
  uniform Plastic *uniform this = uniform new uniform Plastic;
  Plastic__Constructor(this,pigmentColor,eta,roughness);
  return this;
}
