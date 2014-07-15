/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "material.ih"
#include "brdfs/dielectric_reflection.ih"
#include "brdfs/dielectric_transmission.ih"

struct Dielectric
{
  PathTraceMaterial base;
  Medium mediumOutside;
  Medium mediumInside;
  DielectricReflection reflection_io;
  DielectricReflection reflection_oi;
  DielectricTransmission transmission_io;
  DielectricTransmission transmission_oi;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

void Dielectric__shade(const uniform PathTraceMaterial *uniform _this,
                       const Ray&                  ray,      
                       const Medium&               currentMedium,
                       const DifferentialGeometry& dg,           
                       uniform CompositedBRDF&     brdfs)        
{
  uniform const Dielectric *uniform this = (uniform const Dielectric *uniform)_this;

  /*! the ray transitions from outside to inside */
  if (eq(currentMedium,this->mediumOutside)) {
    CompositedBRDF__add(&brdfs,&this->reflection_oi.base);
    CompositedBRDF__add(&brdfs,&this->transmission_oi.base);
  }
  
  /*! the ray transitions from inside to outside */
  else {
    CompositedBRDF__add(&brdfs,&this->reflection_io.base);
    CompositedBRDF__add(&brdfs,&this->transmission_io.base);
  }
}

void Dielectric__selectNextMedium(const uniform PathTraceMaterial *uniform _this,
                                  varying Medium &lastMedium) 
{
  const uniform Dielectric *uniform this = (const uniform Dielectric *uniform)_this;
  
  if (eq(lastMedium,this->mediumOutside)) lastMedium = this->mediumInside;
  else                                    lastMedium = this->mediumOutside;
}

void Dielectric__Constructor(uniform Dielectric* uniform this,
                             const uniform float etaOutside,
                             const uniform vec3f& transmissionOutside,
                             const uniform float etaInside,
                             const uniform vec3f& transmissionInside) 
{
  PathTraceMaterial__Constructor(&this->base,// PathTraceMaterial__Destructor,
                                 Dielectric__shade,Dielectric__selectNextMedium);
  this->mediumOutside.eta          = etaOutside;
  this->mediumOutside.transmission = transmissionOutside;
  this->mediumInside.eta           = etaInside;
  this->mediumInside.transmission  = transmissionInside;
  DielectricReflection__Constructor  (&this->reflection_io  ,etaInside ,etaOutside);
  DielectricReflection__Constructor  (&this->reflection_oi  ,etaOutside,etaInside);
  DielectricTransmission__Constructor(&this->transmission_io,etaInside ,etaOutside);
  DielectricTransmission__Constructor(&this->transmission_oi,etaOutside,etaInside);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Dielectric_create(const uniform float etaOutside,
                                     const uniform vec3f& transmissionOutside,
                                     const uniform float etaInside,
                                     const uniform vec3f& transmissionInside) 
{
  uniform Dielectric *uniform mat = uniform new uniform Dielectric;
  Dielectric__Constructor(mat,etaOutside,transmissionOutside,etaInside,transmissionInside);
  return mat;
}
