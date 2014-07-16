/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "pointlight.ih"

//! Construct an ispc-side PointLight object
void PointLight_Constructor( uniform PointLight *uniform THIS,
                             void *uniform cppEquivalent,
                             const uniform vec3f &position,
                             const uniform vec3f &color,
                             const uniform float range)
{
  Light_Constructor(&THIS->base, cppEquivalent);
  THIS->position = position;
  THIS->color = color;
  THIS->range = range;
}

//! Destroy an ispc-side PointLight object and its base object
void PointLight_Destructor(uniform PointLight *uniform THIS)
{
  Light_Destructor(&THIS->base);
}

//! Create an ispc-side PointLight object (called from c++ side)
export void *uniform PointLight_create(void *uniform cppEquivalent)
{
  uniform PointLight *uniform light = uniform new uniform PointLight;
  PointLight_Constructor(light, cppEquivalent, make_vec3f(0.f, 0.f, 0.f), make_vec3f(1.f, 1.f, 1.f), -1.f);
  return light;
}

//! Destroy an ispc-side PointLight object (called from c++ side)
export void PointLight_destroy(void *uniform _light)
{
  uniform PointLight *uniform light = (uniform PointLight *uniform)_light;
  PointLight_Destructor(light);
  delete light;
}

//! Set the parameters of an ispc-side PointLight object (called from c++ side)
export void PointLight_set(void *uniform _light,
                           const uniform vec3f &position,
                           const uniform vec3f &color,
                           const uniform float range)
{
  uniform PointLight *uniform light = (uniform PointLight *uniform)_light;
  light->position = position;
  light->color = color;
  light->range = range;
}
