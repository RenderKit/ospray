#include "light.ih"
#include "samplers/shapesampler.ih"

struct AmbientLight
{
  EnvironmentLight base;
  vec3f L;
};

uniform EnvironmentLight* uniform AmbientLight__New(const uniform vec3f& L);

uniform Light* uniform AmbientLight__transform(const uniform Light *uniform _this, 
                                               const uniform AffineSpace3f& xfm) 
{
  const uniform AmbientLight* uniform this = (const uniform AmbientLight* uniform)_this;
  return (uniform Light* uniform) AmbientLight__New(this->L);
}

varying vec3f AmbientLight__Le(const uniform EnvironmentLight* uniform _this, varying const vec3f& wo) 
{
  const uniform AmbientLight* uniform this = (const uniform AmbientLight* uniform)_this;
  return this->L;
}

varying vec3f AmbientLight__eval(const uniform Light* uniform _this, 
                                 varying const DifferentialGeometry& dg, 
                                 varying const vec3f& wi) 
{
  const uniform AmbientLight* uniform this = (const uniform AmbientLight* uniform)_this;
  return this->L;
}

varying vec3f AmbientLight__sample(const uniform Light* uniform _this,
                                   varying const DifferentialGeometry& dg, 
                                   varying Sample3f& wi,
                                   varying float& tMax,
                                   varying const vec2f& s) 
{
  const uniform AmbientLight* uniform this = (const uniform AmbientLight* uniform)_this;
  wi = cosineSampleHemisphere(//.5f,.5f,
                              s.x, s.y, 
                              dg.Ns);
  tMax = 1e20f;
  return this->L;
}

void AmbientLight__Constructor(uniform AmbientLight* uniform this, const uniform vec3f L) 
{
  EnvironmentLight__Constructor(&this->base,//Light__Destructor,
                                ENV_LIGHT,
                                AmbientLight__transform,NULL,AmbientLight__eval,AmbientLight__sample,
                                AmbientLight__Le);
  this->L = L;
}

uniform EnvironmentLight* uniform AmbientLight__New(const uniform vec3f& L)
{
  uniform AmbientLight* uniform light = uniform new uniform AmbientLight;
  AmbientLight__Constructor(light,L);
  return (EnvironmentLight*)light;
}

export void* uniform AmbientLight__new(const uniform vec3f& L) {
  return AmbientLight__New(L);
}
