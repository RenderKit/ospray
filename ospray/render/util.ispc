/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospray/render/util.ih"

uniform float precomputedHalton[3][NUM_PRECOMPUTED_HALTON_VALUES];
uniform bool  precomputedHalton_initialized = false;

inline float radicalInverse(const uint32 _v, const uint32 prime)  
{
  uint32 v = _v;
  float nom = 0.f;
  float den = 1.f;
  while(v != 0) {
    uint32 r = v % prime;    
    v = v / prime;
    
    nom = nom * prime + r;
    den = den * prime;
  }
  float ret = nom / den;
  return ret;
}

static void precomputedHalton_create() {
  if (precomputedHalton_initialized) 
    return;
  foreach (i=0 ... NUM_PRECOMPUTED_HALTON_VALUES) {
    precomputedHalton[0][i] = radicalInverse(i,2);
    precomputedHalton[1][i] = radicalInverse(i,3);
    precomputedHalton[2][i] = radicalInverse(i,5);
  }
  precomputedHalton_initialized = true;
};


