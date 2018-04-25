// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

export void Geometry_setMaterialList(void *uniform _geo, 
                                     void *uniform _matList)
{
  uniform Geometry *uniform geo = (uniform Geometry *uniform)_geo;
  geo->materialList = (uniform Material *uniform *)_matList;
}

int32 Geometry_getMaterialID(
    const Geometry *uniform const _self
    , const int32 primID
    )
{
  return -1;
}

//! constructor for ispc-side Geometry object
static void Geometry_Constructor(uniform Geometry *uniform geometry,
                                 void *uniform cppEquivalent,
                                 uniform Geometry_postIntersectFct postIntersect,
                                 uniform Model *uniform model,
                                 uniform int32  geomID,
                                 uniform Material *uniform *uniform materialList
                                 )
{
  geometry->cppEquivalent = cppEquivalent;
  geometry->numPrimitives = 0;
  geometry->model = model;
  geometry->materialList = materialList;
  geometry->geomID = geomID;
  geometry->postIntersect = postIntersect;
  geometry->getMaterialID = Geometry_getMaterialID;
  geometry->sampleArea = NULL;
  geometry->getAreas = NULL;
}
