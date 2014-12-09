#include "Material.ih"
#include "brdfs/DielectricReflection.ih"
#include "brdfs/DielectriclayerLambertian.ih"
#include "brdfs/DielectriclayerMicrofacetMetal.ih"

struct MetallicPaint
{
  PathTraceMaterial base;
  DielectricReflection              reflection;
  DielectricLayerLambertian         paint;
  DielectricLayerMicrofacetMetal    glitter;
  bool                              hasGlitter;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

void MetallicPaint__shade(const uniform PathTraceMaterial *uniform _this,
                          const Ray&                  ray,             
                          const Medium&               currentMedium,   
                          const DifferentialGeometry& dg,              
                          uniform CompositedBRDF&     brdfs)           
{
  uniform const MetallicPaint *uniform this = (uniform const MetallicPaint *uniform)_this;
  CompositedBRDF__add(&brdfs,&this->reflection.base);
  CompositedBRDF__add(&brdfs,&this->paint.base);
  if (this->hasGlitter) CompositedBRDF__add(&brdfs,&this->glitter.base);
}

inline void MetallicPaint__Constructor(uniform MetallicPaint *uniform this,
                                       const uniform vec3f& shadeColor,
                                       const uniform vec3f& glitterColor,
                                       const uniform float glitterSpread,
                                       const uniform float eta)
{
  PathTraceMaterial__Constructor(&this->base,// PathTraceMaterial__Destructor,
                                 MetallicPaint__shade,NULL);
  DielectricReflection__Constructor(&this->reflection, 1.0f, eta);
  DielectricLayerLambertian__Constructor(&this->paint, make_vec3f(1.0f), 1.0f, eta, make_Lambertian(shadeColor));

  uniform vec3f etaAluminium = make_vec3f(0.62f,0.62f,0.62f);
  uniform vec3f kAluminium   = make_vec3f(4.8f,4.8f,4.8f);
  DielectricLayerMicrofacetMetal__Constructor(&this->glitter,
                                              make_vec3f(1.f), 1.f, eta, 
                                              make_MicrofacetMetal(glitterColor,
                                                                   make_FresnelConductor(etaAluminium,kAluminium),
                                                                   make_PowerCosineDistribution(rcp(glitterSpread))));
  this->hasGlitter = (glitterSpread != 0.0f & ne(glitterColor,make_vec3f(0.0f)));
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_MetallicPaint_create(const uniform vec3f& shadeColor,
                                                     const uniform vec3f& glitterColor,
                                                     const uniform float glitterSpread,
                                                     const uniform float eta)
{
  uniform MetallicPaint *uniform this = uniform new uniform MetallicPaint;
  MetallicPaint__Constructor(this,shadeColor,glitterColor,glitterSpread,eta);
  return this;
}
