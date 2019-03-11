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

typedef uniform Geometry *uniform uniGeomPtr;
typedef uniform Material *uniform uniMaterialPtr;
typedef uniform Volume   *uniform uniVolumePtr;

export void *uniform World_create(void *uniform cppE)
{
  uniform World *uniform model = uniform new uniform World;
  model->cppEquivalent     = cppE;
  model->embreeSceneHandle = NULL;
  model->geometry          = NULL;
  model->volumes           = NULL;
  return (void *uniform)model;
}

export void World_cleanup(void *uniform _model)
{
  uniform World *uniform model = (uniform World *uniform)_model;
  if (model->geometry) delete[] model->geometry;
  if (model->volumes)  delete[] model->volumes;
}

export void World_init(void *uniform _model,
                       void *uniform embreeDevice,
                       uniform int32 embreeSceneFlags,
                       uniform int32 numGeometries,
                       uniform int32 numVolumes)
{
  uniform World *uniform model = (uniform World *uniform)_model;
  if (model->embreeSceneHandle) {
    rtcReleaseScene(model->embreeSceneHandle);
  }

  model->embreeSceneHandle = rtcNewScene((RTCDevice)embreeDevice);

  rtcSetSceneFlags(model->embreeSceneHandle,
                   (uniform RTCSceneFlags)embreeSceneFlags);

  if (model->geometry) delete[] model->geometry;
  model->geometryCount = numGeometries;
  if (numGeometries > 0)
    model->geometry = uniform new uniform uniGeomPtr[numGeometries];
  else
    model->geometry = NULL;

  if (model->volumes) delete[] model->volumes;
  model->volumeCount = numVolumes;
  if (numVolumes > 0)
    model->volumes = uniform new uniform uniVolumePtr[numVolumes];
  else
    model->volumes = NULL;
}

export void World_setBounds(void *uniform _model,
                            uniform box3f *uniform bounds)
{
  uniform World *uniform model = (uniform World *uniform)_model;
  model->bounds = *bounds;
}

export void *uniform World_getEmbreeSceneHandle(void *uniform _model)
{
  uniform World *uniform model = (uniform World *uniform)_model;
  return (void *uniform)model->embreeSceneHandle;
}

export void World_setGeometry(void *uniform _model,
                              uniform int32 geomID,
                              void *uniform _geom)
{
  uniform World *uniform model = (uniform World *uniform)_model;
  uniform Geometry *uniform geom = (uniform Geometry *uniform)_geom;
  model->geometry[geomID] = geom;
}

export void World_setVolume(void *uniform pointer,
                            uniform int32 index,
                            void *uniform volume)
{
  uniform World *uniform model = (uniform World *uniform) pointer;
  model->volumes[index] = (uniform Volume *uniform) volume;
}

