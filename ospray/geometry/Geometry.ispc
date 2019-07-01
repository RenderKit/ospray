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

#include "Geometry.ih"

//! constructor for ispc-side Geometry object
static void Geometry_Constructor(uniform Geometry *uniform geometry,
                                 void *uniform cppEquivalent,
                                 uniform Geometry_postIntersectFct
                                     postIntersect,
                                 RTCGeometry geom)
{
  geometry->cppEquivalent = cppEquivalent;
  geometry->numPrimitives = 0;
  geometry->geomID        = -1;
  geometry->postIntersect = postIntersect;
  geometry->sampleArea    = NULL;
  geometry->getAreas      = NULL;
}

export void Geometry_set_geomID(void *uniform _self, uniform int32 geomID)
{
  Geometry *uniform self = (Geometry *uniform)_self;

  self->geomID = geomID;
}
