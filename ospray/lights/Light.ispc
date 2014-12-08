/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "Light.ih"

//! Construct a new ispc side light
static void Light_Constructor(uniform Light *uniform light,
                              void *uniform cppEquivalent)
{
  light->cppEquivalent = cppEquivalent;
}

//! Destroy an ispc side light - stub
extern void Light_Destructor(uniform Light *uniform light) {}
