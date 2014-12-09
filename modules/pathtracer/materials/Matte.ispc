/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "material.ih"
#include "brdfs/lambertian.ih"

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
                  uniform CompositedBRDF&     brdfs)          
{
  uniform const Matte *uniform this = (uniform const Matte *uniform)_this;
  CompositedBRDF__add(&brdfs,&this->lambertian.base);
}

inline void Matte__Constructor(uniform Matte *uniform this,
                               const uniform vec3f& reflectance)
{
  PathTraceMaterial__Constructor(&this->base,// PathTraceMaterial__Destructor,
                                 Matte__shade,NULL);
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
