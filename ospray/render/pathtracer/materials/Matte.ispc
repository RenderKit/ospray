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
#include "../brdfs/Lambertian.ih"

struct Matte
{
  PathTraceMaterial base;
  Lambertian lambertian;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

void Matte__shade(const uniform PathTraceMaterial *uniform _this,
                  const Ray&                  ray,            
                  const Medium&               currentMedium,  
                  const DifferentialGeometry& dg,             
                  uniform CompositedBRDF&     brdfs,
                  vec3f& Ns)
{
  uniform const Matte *uniform this = (uniform const Matte *uniform)_this;
  CompositedBRDF__add(&brdfs,&this->lambertian.base);
}

inline void Matte__Constructor(uniform Matte *uniform this,
                               const uniform vec3f& reflectance)
{
  PathTraceMaterial__Constructor(&this->base, Matte__shade, NULL);
  this->lambertian = make_Lambertian(reflectance);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Matte_create(const uniform vec3f& reflectance)
{
  uniform Matte *uniform this = uniform new uniform Matte;
  Matte__Constructor(this,reflectance);
  return this;
}
