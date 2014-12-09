#include "Light.ih"
#include "samplers/ShapeSampler.ih"

struct DirectionalLight
{
  Light base;
  vec3f D;
  vec3f E;
};

DirectionalLight* uniform DirectionalLight__New(const uniform vec3f& D, const uniform vec3f& E);

uniform Light* uniform DirectionalLight__transform(const uniform Light *uniform _this, 
                                                   const uniform AffineSpace3f& xfm) 
{
  const uniform DirectionalLight *uniform this = (const uniform DirectionalLight *uniform)_this;
  return (uniform Light* uniform) DirectionalLight__New(xfmVector(xfm,this->D),this->E);
}

varying vec3f DirectionalLight__eval(const uniform Light *uniform _this, 
                                     varying const DifferentialGeometry &dg, 
                                     varying const vec3f &wi) {
  return make_vec3f(0.0f);
}

varying vec3f DirectionalLight__sample(const uniform Light *uniform _this,
                                       varying const DifferentialGeometry &dg, 
                                       varying Sample3f &wi,
                                       varying float &tMax,
                                       varying const vec2f &s) 
{
  const uniform DirectionalLight *uniform this = (const uniform DirectionalLight *uniform)_this;
  wi = make_Sample3f(neg(this->D),1.0f); 
  tMax = inf; 
  return this->E;
}

void DirectionalLight__Constructor (uniform DirectionalLight *uniform this,
                                    const uniform vec3f D,
                                    const uniform vec3f E)
{
  Light__Constructor(&this->base,Light__Destructor,NORMAL_LIGHT,
                     DirectionalLight__transform,NULL,DirectionalLight__eval,DirectionalLight__sample);
  this->D = normalize(D);
  this->E = E;
}

DirectionalLight* uniform DirectionalLight__New(const uniform vec3f& D, const uniform vec3f& E)
{
  uniform DirectionalLight *uniform this = uniform new uniform DirectionalLight;
  DirectionalLight__Constructor(this,D,E);
  return this;
}

export void* uniform DirectionalLight__new(const uniform vec3f& D, const uniform vec3f& E) {
  return DirectionalLight__New(D,E);
}
