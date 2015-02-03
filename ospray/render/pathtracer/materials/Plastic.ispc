#include "Material.ih"
#include "brdfs/DielectricLayerLambertian.ih"
#include "brdfs/DielectricReflection.ih"
#include "brdfs/MicrofacetPlastic.ih"

struct Plastic
{
  PathTraceMaterial base;
  float roughness;
  DielectricLayerLambertian substrate;
  DielectricReflection reflection;
  MicrofacetPlastic microfacet;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

void Plastic__shade(const uniform PathTraceMaterial *uniform _this,
                  const Ray&                  ray,             
                  const Medium&               currentMedium,   
                  const DifferentialGeometry& dg,              
                  uniform CompositedBRDF&     brdfs)           
{
  const uniform Plastic* uniform this = (const uniform Plastic* uniform)_this;
  CompositedBRDF__add(&brdfs,&this->substrate.base);
  if (this->roughness == 0.0f)  CompositedBRDF__add(&brdfs,&this->reflection.base);
  else                          CompositedBRDF__add(&brdfs,&this->microfacet.base);
}

void Plastic__Constructor(uniform Plastic *uniform this,
                          const uniform vec3f& pigmentColor,
                          const uniform float eta,
                          const uniform float roughness)
{
  PathTraceMaterial__Constructor(&this->base,// PathTraceMaterial__Destructor,
                                 Plastic__shade,NULL);
  this->roughness = roughness;

  DielectricLayerLambertian__Constructor(&this->substrate,
                                         make_vec3f(1.0f), 1.0f, eta, 
                                         make_Lambertian(pigmentColor));

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
