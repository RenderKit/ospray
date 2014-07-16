/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "objpointlight.ih"

//! Construct a new ispc-side OBJPointLight
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

//! Destroy an ispc-side OBJPointLight
void OBJPointLight_Destructor(uniform OBJPointLight *uniform THIS)
{
  PointLight_Destructor(&THIS->base);
}

//! Create a new ispc-side OBJPointLight (called from c++-side)
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

//! Destroy an ispc-side OBJPointLight (called from c++-side)
export void OBJPointLight_destroy(void *uniform _light)
{
  uniform OBJPointLight *uniform light = (uniform OBJPointLight *uniform)_light;
  OBJPointLight_Destructor(light);
  delete light;
}

//! Set members of an ispc-side OBJPointLight (called from c++-side)
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
                              
