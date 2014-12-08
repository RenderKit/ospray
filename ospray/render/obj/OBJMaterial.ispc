/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "OBJMaterial.ih"

export void *uniform OBJMaterial_create(void *uniform cppE)
{
  uniform OBJMaterial *uniform mat = uniform new uniform OBJMaterial;
  mat->base.cppEquivalent = cppE;
  return mat;
}

export void OBJMaterial_destroy(void *uniform _mat)
{
  uniform OBJMaterial *uniform mat = (uniform OBJMaterial *uniform)_mat;
  delete(mat);
}

export void OBJMaterial_set(void *uniform _mat,
                            const void *uniform map_d, 
                            const uniform float &d,
                            const void *uniform map_Kd,
                            const uniform vec3f &Kd,
                            const void *uniform map_Ks,
                            const uniform vec3f &Ks,
                            const void *uniform map_Ns,
                            const uniform float &Ns,
                            const void *uniform map_Bump)
{
  uniform OBJMaterial *uniform THIS = (uniform OBJMaterial *uniform)_mat;
  THIS->map_d = (uniform Texture2D *uniform)map_d;
  THIS->d = d;
  THIS->map_Kd = (uniform Texture2D *uniform)map_Kd;
  THIS->Kd = Kd;
  THIS->map_Ks = (uniform Texture2D *uniform)map_Ks;
  THIS->Ks = Ks;
  THIS->map_Ns = (uniform Texture2D *uniform)map_Ns;
  THIS->Ns = Ns;
  THIS->map_Bump = (uniform Texture2D *uniform)map_Bump;
}
