// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "SimpleAOMaterial.ih"

export void *uniform SimpleAOMaterial_create(void *uniform cppE)
{
  uniform SimpleAOMaterial *uniform mat = uniform new uniform SimpleAOMaterial;
  mat->super.cppEquivalent = cppE;
  return mat;
}

export void SimpleAOMaterial_set(void *uniform _mat,
                                 const uniform vec3f &Kd,
                                 void *uniform _map_Kd)
{
  uniform SimpleAOMaterial *uniform mat =
      (uniform SimpleAOMaterial *uniform)_mat;
  mat->Kd = Kd;
  mat->map_Kd = (uniform Texture2D *uniform)_map_Kd;
}
