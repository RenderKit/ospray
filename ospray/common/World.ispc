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

typedef Volume *uniform uniVolumePtr;

typedef GeometryInstance *uniform uniGeomInstPtr;

export void *uniform World_create(void *uniform cppE)
{
  World *uniform world = uniform new World;

  world->cppEquivalent     = cppE;
  world->embreeSceneHandle = NULL;
  world->geometryInstances = NULL;
  world->volumes           = NULL;

  return (void *uniform)world;
}

export void World_cleanup(void *uniform _world)
{
  uniform World *uniform world = (uniform World * uniform) _world;
  if (world->volumes)
    delete[] world->volumes;
  if (world->geometryInstances)
    delete[] world->geometryInstances;
}

export void World_init(void *uniform _world,
                       void *uniform embreeDevice,
                       uniform int32 embreeSceneFlags,
                       uniform int32 numGeometryInstances,
                       uniform int32 numVolumes)
{
  uniform World *uniform world = (uniform World * uniform) _world;
  if (world->embreeSceneHandle)
    rtcReleaseScene(world->embreeSceneHandle);

  world->embreeSceneHandle = rtcNewScene((RTCDevice)embreeDevice);

  rtcSetSceneFlags(world->embreeSceneHandle,
                   (uniform RTCSceneFlags)embreeSceneFlags);

  if (world->geometryInstances)
    delete[] world->geometryInstances;
  world->geometryInstanceCount = numGeometryInstances;
  if (numGeometryInstances > 0) {
    world->geometryInstances = uniform new uniGeomInstPtr[numGeometryInstances];
  } else {
    world->geometryInstances = NULL;
  }

  if (world->volumes)
    delete[] world->volumes;
  world->volumeCount = numVolumes;
  if (numVolumes > 0)
    world->volumes = uniform new uniVolumePtr[numVolumes];
  else
    world->volumes = NULL;
}

export void World_setBounds(void *uniform _world, box3f *uniform bounds)
{
  World *uniform world = (World * uniform) _world;
  world->bounds        = *bounds;
}

export void *uniform World_getEmbreeSceneHandle(void *uniform _world)
{
  World *uniform world = (World * uniform) _world;
  return (void *uniform)world->embreeSceneHandle;
}

export void World_setGeometryInstance(void *uniform _world,
                                      uniform int32 instID,
                                      void *uniform _instance)
{
  World *uniform world               = (World * uniform) _world;
  GeometryInstance *uniform instance = (GeometryInstance * uniform) _instance;
  world->geometryInstances[instID]   = instance;
}

export void World_setVolume(void *uniform pointer,
                            uniform int32 volumeID,
                            void *uniform volume)
{
  World *uniform world     = (World * uniform) pointer;
  world->volumes[volumeID] = (Volume * uniform) volume;
}
