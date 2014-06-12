#include "material.ih"
#include "brdfs/dielectric_reflection.ih"
#include "brdfs/dielectric_transmission.ih"

struct ThinDielectric
{
  uniform PathTraceMaterial base;
  DielectricReflection reflection;
  ThinDielectricTransmission transmission;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

void ThinDielectric__shade(const uniform PathTraceMaterial *uniform _this,
                       const Ray&                  ray,            
                       const Medium&               currentMedium,  
                       const DifferentialGeometry& dg,             
                       uniform CompositedBRDF&     brdfs)          
{
  uniform const ThinDielectric *uniform this = (uniform const ThinDielectric *uniform)_this;
  CompositedBRDF__add(&brdfs,&this->reflection.base);
  CompositedBRDF__add(&brdfs,&this->transmission.base);
}

void ThinDielectric__Constructor(uniform ThinDielectric* uniform this,
                                 const uniform vec3f& transmission,
                                 const uniform float eta,
                                 const uniform float thickness)
{
  PathTraceMaterial__Constructor(&this->base,// PathTraceMaterial__Destructor,
                                 ThinDielectric__shade,NULL);
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
