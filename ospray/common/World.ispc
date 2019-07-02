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

#include "World.ih"

export void *uniform World_create(void *uniform cppE)
{
  World *uniform world = uniform new World;

  world->cppEquivalent = cppE;
  world->instances     = NULL;

  world->embreeSceneHandleGeometries = NULL;
  world->embreeSceneHandleVolumes    = NULL;

  return (void *uniform)world;
}

export void World_set(void *uniform _self,
                      void *uniform _instances,
                      uniform int32 numInstances,
                      void *uniform embreeGeometryScene,
                      void *uniform embreeVolumeScene)
{
  World *uniform self          = (World * uniform) _self;
  Instance **uniform instances = (Instance * *uniform) _instances;

  self->instances    = instances;
  self->numInstances = numInstances;

  self->embreeSceneHandleGeometries = (RTCScene)embreeGeometryScene;
  self->embreeSceneHandleVolumes    = (RTCScene)embreeVolumeScene;
}
