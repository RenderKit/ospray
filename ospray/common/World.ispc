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

typedef VolumeInstance *uniform uniVolInstPtr;

export void *uniform World_create(void *uniform cppE)
{
  World *uniform world = uniform new World;

  world->cppEquivalent     = cppE;
  world->geometryInstances = NULL;
  world->volumeInstances   = NULL;

  world->embreeSceneHandleGeometries = NULL;
  world->embreeSceneHandleVolumes    = NULL;

  return (void *uniform)world;
}

export void World_cleanup(void *uniform _world)
{
  uniform World *uniform world = (uniform World * uniform) _world;
  if (world->geometryInstances)
    delete[] world->geometryInstances;
  if (world->volumeInstances)
    delete[] world->volumeInstances;
}

export void World_init(void *uniform _world,
                       void *uniform embreeDevice,
                       uniform int32 embreeSceneFlags,
                       uniform int32 numGeometryInstances,
                       uniform int32 numVolumeInstances)
{
  uniform World *uniform world = (uniform World * uniform) _world;
  if (world->embreeSceneHandleGeometries)
    rtcReleaseScene(world->embreeSceneHandleGeometries);
  if (world->embreeSceneHandleVolumes)
    rtcReleaseScene(world->embreeSceneHandleVolumes);

  world->embreeSceneHandleGeometries = rtcNewScene((RTCDevice)embreeDevice);
  world->embreeSceneHandleVolumes    = rtcNewScene((RTCDevice)embreeDevice);

  rtcSetSceneFlags(world->embreeSceneHandleGeometries,
                   (uniform RTCSceneFlags)embreeSceneFlags);
  rtcSetSceneFlags(world->embreeSceneHandleVolumes,
                   (uniform RTCSceneFlags)embreeSceneFlags);

  if (world->geometryInstances)
    delete[] world->geometryInstances;
  world->geometryInstanceCount = numGeometryInstances;
  if (numGeometryInstances > 0) {
    world->geometryInstances = uniform new uniGeomInstPtr[numGeometryInstances];
  } else {
    world->geometryInstances = NULL;
  }

  if (world->volumeInstances)
    delete[] world->volumeInstances;
  world->volumeInstanceCount = numVolumeInstances;
  if (numVolumeInstances > 0) {
    world->volumeInstances = uniform new uniVolInstPtr[numVolumeInstances];
  } else {
    world->volumeInstances = NULL;
  }
}

export void World_setBounds(void *uniform _world, box3f *uniform bounds)
{
  World *uniform world = (World * uniform) _world;
  world->bounds        = *bounds;
}

export void *uniform World_getEmbreeSceneHandleGeometries(void *uniform _world)
{
  World *uniform world = (World * uniform) _world;
  return (void *uniform)world->embreeSceneHandleGeometries;
}

export void *uniform World_getEmbreeSceneHandleVolumes(void *uniform _world)
{
  World *uniform world = (World * uniform) _world;
  return (void *uniform)world->embreeSceneHandleVolumes;
}

export void World_setGeometryInstance(void *uniform _world,
                                      uniform int32 instID,
                                      void *uniform _instance)
{
  World *uniform world               = (World * uniform) _world;
  GeometryInstance *uniform instance = (GeometryInstance * uniform) _instance;
  world->geometryInstances[instID]   = instance;
}

export void World_setVolumeInstance(void *uniform _world,
                                    uniform int32 instID,
                                    void *uniform _instance)
{
  World *uniform world             = (World * uniform) _world;
  VolumeInstance *uniform instance = (VolumeInstance * uniform) _instance;
  world->volumeInstances[instID]   = instance;
}
