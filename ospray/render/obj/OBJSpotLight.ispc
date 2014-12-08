/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "OBJSpotLight.ih"

//! Construct a new ispc-side OBJSpotLight
extern void OBJSpotLight_Constructor( uniform OBJSpotLight *uniform THIS,
                                      void *uniform cppEquivalent,
                                      const uniform vec3f &position,
                                      const uniform vec3f &direction,
                                      const uniform vec3f &color,
                                      const uniform float range,
                                      const uniform float halfAngle,
                                      const uniform float constantAttenuation,
                                      const uniform float linearAttenuation,
                                      const uniform float quadraticAttenuation)
{


  SpotLight_Constructor(&THIS->base,
                        cppEquivalent,
                        position,
                        direction,
                        color,
                        range,
                        halfAngle);

  THIS->constantAttenuation = constantAttenuation;
  THIS->linearAttenuation = linearAttenuation;
  THIS->quadraticAttenuation = quadraticAttenuation;
}

//! Destruct an ispc-side OBJSpotLight
extern void OBJSpotLight_Destructor(uniform OBJSpotLight *uniform THIS) 
{
  SpotLight_Destructor(&THIS->base);
}

//! Create a new ispc-side OBJSpotLight (called from c++)
export void *uniform OBJSpotLight_create(void *uniform cppEquivalent)
{
  uniform OBJSpotLight *uniform light = uniform new uniform OBJSpotLight;
  OBJSpotLight_Constructor(light,
                           cppEquivalent,
                           make_vec3f(0.f, 0.f, 0.f),
                           make_vec3f(1.f, 1.f, 1.f),
                           make_vec3f(1.f, 1.f, 1.f),
                           -1.f,
                           -1.f,
                           -1.f,
                           -1.f,
                           -1.f);
  return light;
}

//! Destroy an ispc-side OBJSpotLight (called from c++)
export void OBJSpotLight_destroy(void *uniform _light)
{
  uniform OBJSpotLight *uniform light = (uniform OBJSpotLight *uniform)_light;
  OBJSpotLight_Destructor(light);
}

//! Set data for ispc-side OBJSpotLight (called from c++)
export void OBJSpotLight_set( void *uniform _light,
                              const uniform vec3f &position,
                              const uniform vec3f &direction,
                              const uniform vec3f &color,
                              const uniform float range,
                              const uniform float halfAngle,
                              const uniform float constantAttenuation,
                              const uniform float linearAttenuation,
                              const uniform float quadraticAttenuation)
{
  uniform OBJSpotLight *uniform light = (uniform OBJSpotLight *uniform)_light;
  light->base.position        = position;
  light->base.direction       = direction;
  light->base.color           = color;
  light->base.range           = range;
  light->base.halfAngle       = halfAngle;

  light->constantAttenuation  = constantAttenuation;
  light->linearAttenuation    = linearAttenuation;
  light->quadraticAttenuation = quadraticAttenuation;
}
