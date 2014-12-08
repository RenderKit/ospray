/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "SpotLight.ih"

//!< Construct a new ispc-side SpotLight object
void SpotLight_Constructor(uniform SpotLight *uniform THIS,
                                  void *uniform cppEquivalent,
                                  const uniform vec3f &position,
                                  const uniform vec3f &direction,
                                  const uniform vec3f &color,
                                  const uniform float range,
                                  const uniform float halfAngle)
{
  Light_Constructor(&THIS->base, cppEquivalent);
  THIS->position             = position;
  THIS->direction            = direction;
  THIS->color                = color;
  THIS->range                = range;
  THIS->halfAngle            = halfAngle;
  THIS->angularLimit         = cos((halfAngle * M_PI) / 180.0f);
  THIS->base.computeRadiance = SpotLight_computeRadiance;
}

//!< Destruct an ispc-side SpotLight object
void SpotLight_Destructor(uniform SpotLight *uniform THIS)
{
  Light_Destructor(&THIS->base);
}

//! Compute the radiance at a point from a sample on the light sans occluders, return the extant light vector and distance.
varying vec3f SpotLight_computeRadiance(void *uniform pointer, const varying vec3f &coordinates, varying vec3f &direction, varying float &distance) {

    //! Cast to the actual Light subtype.
    const SpotLight *uniform light = (SpotLight *uniform) pointer;

    //! Extant light vector from the hit point.
    const vec3f L = light->position - coordinates;

    //! Distance from the hit point to the sample point on the light.
    distance = length(L);

    //! Normalized light vector.
    direction = L / distance;

    //! Cosine of the negated light direction and light vector.
    const float offAngle = dot(negate(light->direction), direction);

    //! Angular attenuation factor.
    const float angularAttenuation = (offAngle - light->angularLimit) / (1.0f - light->angularLimit);

    //! Radiance attentuated by distance and angle, and clipped to the angular limit of the spot.
    return((offAngle <= light->angularLimit) ? light->color / (distance * distance) * angularAttenuation : make_vec3f(0.0f));

}

//!< Create an ispc-side SpotLight object (called from c++)
export void *uniform SpotLight_create(void *uniform cppEquivalent)
{
  uniform SpotLight *uniform light = uniform new uniform SpotLight;
  SpotLight_Constructor(light,
                        cppEquivalent,
                        make_vec3f(0.f, 0.f, 0.f),
                        make_vec3f(1.f, 1.f, 1.f),
                        make_vec3f(1.f, 1.f, 1.f),
                        -1.f,
                        -1.f);
  return light;
}

//!< Destroy an ispc-side SpotLight object (called from c++)
export void SpotLight_destroy(void *uniform _light)
{
  uniform SpotLight *uniform light = (uniform SpotLight *uniform)_light;
  SpotLight_Destructor(light);
  delete light;
}

//! Set the parameters of an ispc-side SpotLight object (called from c++)
export void SpotLight_set(void *uniform _light,
                          const uniform vec3f &position,
                          const uniform vec3f &direction,
                          const uniform vec3f &color,
                          const uniform float range,
                          const uniform float halfAngle)
{
  uniform SpotLight *uniform light = (uniform SpotLight *uniform)_light;
  light->position       = position;
  light->direction      = direction;
  light->color          = color;
  light->range          = range;
  light->halfAngle      = halfAngle;
}
