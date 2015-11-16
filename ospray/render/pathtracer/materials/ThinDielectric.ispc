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
#include "../bsdfs/DielectricReflection.ih"
#include "../bsdfs/DielectricTransmission.ih"

struct ThinDielectric
{
  uniform PathTraceMaterial base;
  DielectricReflection reflection;
  ThinDielectricTransmission transmission;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

/*
void ThinDielectric__shade(const uniform PathTraceMaterial *uniform _this,
                       const Ray&                  ray,
                       const Medium&               currentMedium,
                       const DifferentialGeometry& dg,
                       uniform CompositedBSDF&     brdfs,
                       vec3f& Ns)
{
  uniform const ThinDielectric *uniform this = (uniform const ThinDielectric *uniform)_this;
  CompositedBSDF_add(&brdfs,&this->reflection.base);
  CompositedBSDF_add(&brdfs,&this->transmission.base);
}
*/

void ThinDielectric__Constructor(uniform ThinDielectric* uniform this,
                                 const uniform vec3f& transmission,
                                 const uniform float eta,
                                 const uniform float thickness)
{
  //PathTraceMaterial_Constructor(&this->base, ThinDielectric__shade, NULL);
  DielectricReflection__Constructor      (&this->reflection  , 1.0f, eta);
  ThinDielectricTransmission__Constructor(&this->transmission, 1.0f, eta, transmission, thickness);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_ThinDielectric_create(const uniform vec3f& transmission,
                                                      const uniform float eta,
                                                      const uniform float thickness)
{
  uniform ThinDielectric *uniform this = uniform new uniform ThinDielectric;
  ThinDielectric__Constructor(this,transmission,eta,thickness);
  return this;
}
