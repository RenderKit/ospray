/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "material.ih"
#include "brdfs/conductor.ih"
#include "brdfs/microfacet_metal.ih"

struct Metal
{
  PathTraceMaterial base;
  float roughness;
  Conductor conductor;
  MicrofacetMetal microfacetMetal;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

void Metal__shade(const uniform PathTraceMaterial *uniform _this,
                  const Ray&                  ray,       
                  const Medium&               currentMedium,
                  const DifferentialGeometry& dg,           
                  uniform CompositedBRDF&     brdfs)        
{
  const uniform Metal* uniform this = (const uniform Metal* uniform)_this;
  if (this->roughness == 0.0f)  CompositedBRDF__add(&brdfs,&this->conductor.base);
  else                          CompositedBRDF__add(&brdfs,&this->microfacetMetal.base);
}

void Metal__Constructor(uniform Metal *uniform this,
                        const uniform vec3f& reflectance,
                        const uniform vec3f& eta,
                        const uniform vec3f& k,
                        const uniform float roughness)
{
  PathTraceMaterial__Constructor(&this->base,// PathTraceMaterial__Destructor,
                                 Metal__shade,NULL);
  this->roughness = roughness;
  Conductor__Constructor(&this->conductor,reflectance, eta, k);
  MicrofacetMetal__Constructor(&this->microfacetMetal,
                               reflectance, 
                               make_FresnelConductor(eta,k), 
                               make_PowerCosineDistribution(rcp(roughness)));
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Metal_create(const uniform vec3f& reflectance,
                                             const uniform vec3f& eta,
                                             const uniform vec3f& k,
                                             const uniform float roughness)
{
  uniform Metal *uniform this = uniform new uniform Metal;
  Metal__Constructor(this,reflectance,eta,k,roughness);
  return this;
}
