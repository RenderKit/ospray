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

#include "Group.ih"

export void *uniform Group_create(void *uniform cppE)
{
  Group *uniform group = uniform new Group;

  group->cppEquivalent = cppE;

  group->geometricModels    = NULL;
  group->numGeometricModels = 0;

  group->volumetricModels    = NULL;
  group->numVolumetricModels = 0;

  return (void *uniform)group;
}

export void Group_set(void *uniform _group,
                      void *uniform _geomModels,
                      uniform int32 numGeomModels,
                      void *uniform _volModels,
                      uniform int32 numVolModels)
{
  Group *uniform group = (Group * uniform) _group;

  group->geometricModels    = (GeometricModel * *uniform) _geomModels;
  group->numGeometricModels = numGeomModels;

  group->volumetricModels    = (VolumetricModel * *uniform) _volModels;
  group->numVolumetricModels = numVolModels;
}
