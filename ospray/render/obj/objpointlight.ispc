#include "objpointlight.ih"

void OBJPointLight_Constructor(uniform OBJPointLight *uniform THIS,
                               void *uniform cppEquivalent,
                               const uniform vec3f &position,
                               const uniform vec3f &color,
                               const uniform float range,
                               const uniform float constantAttenuation,
                               const uniform float linearAttenuation,
                               const uniform float quadraticAttenuation)
{
  PointLight_Constructor(&THIS->base, cppEquivalent, position, color, range);
  THIS->constantAttenuation = constantAttenuation;
  THIS->linearAttenuation = linearAttenuation;
  THIS->quadraticAttenuation = quadraticAttenuation;
}

void OBJPointLight_Destructor(uniform OBJPointLight *uniform THIS)
{
  PointLight_Destructor(&THIS->base);
}

export void *uniform OBJPointLight_create(void *uniform cppEquivalent)
{
  uniform OBJPointLight *uniform light = uniform new uniform OBJPointLight;
  OBJPointLight_Constructor(light,
                            cppEquivalent,
                            make_vec3f(0.f,0.f,0.f),
                            make_vec3f(1.f,1.f,1.f),
                            -1.f,
                            -1.f,
                            -1.f,
                            -1.f);
  return light;
}

export void OBJPointLight_destroy(void *uniform _light)
{
  uniform OBJPointLight *uniform light = (uniform OBJPointLight *uniform)_light;
  OBJPointLight_Destructor(light);
}

export void OBJPointLight_set( void *uniform _light,
                               const uniform vec3f &position,
                               const uniform vec3f &color,
                               const uniform float range,
                               const uniform float constantAttenuation,
                               const uniform float linearAttenuation,
                               const uniform float quadraticAttenuation)
{
  uniform OBJPointLight *uniform light = (uniform OBJPointLight *uniform)_light;
  light->base.position = position;
  light->base.color = color;
  light->base.range = range;
  light->constantAttenuation = constantAttenuation;
  light->linearAttenuation = linearAttenuation;
  light->quadraticAttenuation = quadraticAttenuation;
}
                              
