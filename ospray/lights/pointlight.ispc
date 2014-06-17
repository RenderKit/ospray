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
}
