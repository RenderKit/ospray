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

typedef GeometricModel *uniform uniGeomInstPtr;

typedef VolumetricModel *uniform uniVolInstPtr;

export void *uniform World_create(void *uniform cppE)
{
  World *uniform world = uniform new World;

  world->cppEquivalent   = cppE;
  world->geometricModels = NULL;
  world->volumetricModels = NULL;

  world->embreeSceneHandleGeometries = NULL;
  world->embreeSceneHandleVolumes    = NULL;

  return (void *uniform)world;
}

export void World_cleanup(void *uniform _world)
{
  uniform World *uniform world = (uniform World * uniform) _world;
  if (world->geometricModels)
    delete[] world->geometricModels;
  if (world->volumetricModels)
    delete[] world->volumetricModels;
}

export void World_init(void *uniform _world,
                       void *uniform embreeDevice,
                       uniform int32 embreeSceneFlags,
                       uniform int32 numgeometricModels,
                       uniform int32 numvolumetricModels)
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

  if (world->geometricModels)
    delete[] world->geometricModels;
  world->GeometricModelCount = numgeometricModels;
  if (numgeometricModels > 0) {
    world->geometricModels = uniform new uniGeomInstPtr[numgeometricModels];
  } else {
    world->geometricModels = NULL;
  }

  if (world->volumetricModels)
    delete[] world->volumetricModels;
  world->VolumetricModelCount = numvolumetricModels;
  if (numvolumetricModels > 0) {
    world->volumetricModels = uniform new uniVolInstPtr[numvolumetricModels];
  } else {
    world->volumetricModels = NULL;
  }
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

export void World_setGeometricModel(void *uniform _world,
                                    uniform int32 instID,
                                    void *uniform _instance)
{
  World *uniform world             = (World * uniform) _world;
  GeometricModel *uniform instance = (GeometricModel * uniform) _instance;
  world->geometricModels[instID]   = instance;
}

export void World_setVolumetricModel(void *uniform _world,
                                    uniform int32 instID,
                                    void *uniform _instance)
{
  World *uniform world             = (World * uniform) _world;
  VolumetricModel *uniform instance = (VolumetricModel * uniform) _instance;
  world->volumetricModels[instID]   = instance;
}
