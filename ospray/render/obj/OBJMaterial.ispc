// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
