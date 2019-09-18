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

#include "DistributedWorld.ih"
#include "common/World.ih"

export void *uniform DistributedWorld_create(void *uniform cppE)
{
  uniform DistributedWorld *uniform world =
      uniform new uniform DistributedWorld;
  world->super.cppEquivalent = cppE;
  world->super.instances = NULL;

  world->super.embreeSceneHandleGeometries = NULL;
  world->super.embreeSceneHandleVolumes = NULL;
  return (void *uniform)world;
}

export void DistributedWorld_set(
    void *uniform _world, void *uniform _regions, uniform int numRegions)
{
  uniform DistributedWorld *uniform world =
      (uniform DistributedWorld * uniform) _world;
  world->regions = (uniform Region * uniform) _regions;
  world->numRegions = numRegions;
}
