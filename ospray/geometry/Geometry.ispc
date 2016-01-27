// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "Geometry.ih"

export void Geometry_setMaterial(void *uniform _geo, 
                                 void *uniform _mat)
{
  uniform Geometry *uniform geo = (uniform Geometry *uniform)_geo;
  geo->material = (uniform Material *uniform)_mat;
}

//! constructor for ispc-side Geometry object
static void Geometry_Constructor(uniform Geometry *uniform geometry,
                                 void *uniform cppEquivalent,
                                 uniform Geometry_postIntersectFct postIntersect,
                                 uniform Model *uniform model,
                                 uniform int32  geomID,
                                 uniform Material *uniform material
                                 )
{
  geometry->cppEquivalent = cppEquivalent;
  geometry->model    = model;
  geometry->material = material;
  geometry->geomID   = geomID;
  geometry->postIntersect = postIntersect;
}
