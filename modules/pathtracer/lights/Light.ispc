#include "Light.ih"

void Light__Constructor(uniform Light* uniform this,
                        // uniform DestructorFunc destructor,
                        uniform LightType type,
                        uniform LightTransformFunc transform,
                        uniform ShapeFunc shape,
                        uniform EvalFunc eval,
                        uniform SampleFunc sample)
{
  LOG(print("Light__Constructor\n"));
  // RefCount__Constructor(&this->base,destructor);
  this->type = type;
  this->transform = transform;
  this->shape = shape;
  this->eval = eval;
  this->sample = sample;
}

void AreaLight__Constructor(uniform AreaLight* uniform this,
                            // uniform DestructorFunc destructor,
                            uniform LightType type,
                            uniform LightTransformFunc transform,
                            uniform ShapeFunc shape,
                            uniform EvalFunc eval,
                            uniform SampleFunc sample_,
                            uniform AreaLeFunc Le)
{
  Light__Constructor(&this->base,//destructor,
                     type,transform,shape,eval,sample_);
  this->Le = Le;
}

void EnvironmentLight__Constructor(uniform EnvironmentLight* uniform this,
                                   // uniform DestructorFunc destructor,
                                   uniform LightType type,
                                   uniform LightTransformFunc transform,
                                   uniform ShapeFunc shape,
                                   uniform EvalFunc eval,
                                   uniform SampleFunc sample_,
                                   uniform EnvironmentLeFunc Le)
{
  Light__Constructor(&this->base,// destructor,
                     type,transform,shape,eval,sample_);
  this->Le = Le;
}

export uniform LightType Light__getType(void* uniform _this)
{
  uniform Light *uniform this = (uniform Light *uniform) _this;
  return this->type;
}

export void* uniform Light__transform(void* uniform _this,
                                      uniform const vec3f& vx, 
                                      uniform const vec3f& vy, 
                                      uniform const vec3f& vz, 
                                      uniform const vec3f& p)
{
  const uniform Light *uniform this = (const uniform Light *uniform) _this;
  const uniform AffineSpace3f xfm = make_AffineSpace3f(vx,vy,vz,p);
  return this->transform(this,xfm);
}

export void* uniform Light__shape(void *uniform _this)
{
  const uniform Light *uniform this = (const uniform Light *uniform) _this;
  if (!this->shape) return NULL;
  return this->shape(this);
}
