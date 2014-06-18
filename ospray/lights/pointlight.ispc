#include "pointlight.ih"

void PointLight_Constructor( uniform PointLight *uniform THIS,
                                    const uniform vec3f &position,
                                    const uniform vec3f &color,
                                    const uniform float range)
{
  THIS->position = position;
  THIS->color = color;
  THIS->range = range;
}

void PointLight_Destructor(uniform PointLight *uniform THIS)
{
  Light_Destructor(&THIS->base);
}

export void *uniform PointLight_create(void *uniform cppEquivalent)
{
  uniform PointLight *uniform light = uniform new uniform PointLight;
  Light_Constructor(&light->base, cppEquivalent);
  PointLight_Constructor(light, make_vec3f(0.f, 0.f, 0.f), make_vec3f(1.f, 1.f, 1.f), -1.f);
  return light;
}

export void PointLight_destroy(void *uniform _light)
{
  uniform PointLight *uniform light = uniform new uniform PointLight;
  PointLight_Destructor(light);
  delete light;
}

export void PointLight_set(void *uniform _light,
                           const uniform vec3f &position,
                           const uniform vec3f &color,
                           const uniform float range)
{
  uniform PointLight *uniform light = (uniform PointLight *uniform)_light;
  PointLight_Constructor(light, position, color, range);
}
