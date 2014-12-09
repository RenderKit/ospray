// pathtracer
#include "Light.ih"
#include "samplers/ShapeSampler.ih"
// ospray
#include "math/affine.ih"

struct PointLight
{
  uniform Light base;
  uniform vec3f P;
  uniform vec3f I;
};

PointLight* uniform PointLight__New(const uniform vec3f& P, const uniform vec3f& I);

uniform Light* uniform PointLight__transform(const uniform Light *uniform _this, 
                                             const uniform AffineSpace3f& xfm) 
{
  const uniform PointLight *uniform this = (const uniform PointLight *uniform)_this;
  return (uniform Light* uniform) PointLight__New(xfmPoint(xfm,this->P),this->I);
}

varying vec3f PointLight__eval(const uniform Light *uniform _this, 
                               varying const DifferentialGeometry &dg, 
                               varying const vec3f &wi) 
{
  return make_vec3f(0.0f);
}

varying vec3f PointLight__sample(const uniform Light *uniform _this,
                                   varying const DifferentialGeometry &dg, 
                                   varying Sample3f &wi,
                                   varying float &tMax,
                                   varying const vec2f &s) 
{
  const uniform PointLight *uniform this = (const uniform PointLight *uniform)_this;
  vec3f d = sub(this->P, dg.P);
  float distance = length(d);
  wi = make_Sample3f(mul(d,rcp(distance)), distance*distance);
  tMax = distance;
  return this->I;
}

void PointLight__Constructor(uniform PointLight* uniform this, const uniform vec3f& P, const uniform vec3f& I)
{
  Light__Constructor(&this->base,//Light__Destructor,
                     NORMAL_LIGHT,
                     PointLight__transform,NULL,PointLight__eval,PointLight__sample);
  this->P = P; 
  this->I = I;
}

PointLight* uniform PointLight__New(const uniform vec3f& P, const uniform vec3f& I)
{
  uniform PointLight *uniform this = uniform new uniform PointLight;
  PointLight__Constructor(this,P,I);
  return this;
}

export void* uniform PointLight__new(const uniform vec3f& P, const uniform vec3f& I) {
  return PointLight__New(P, I);
}
