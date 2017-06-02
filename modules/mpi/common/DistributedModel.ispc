// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "common/Model.ih"
#include "embree2/rtcore_scene.isph"

typedef uniform Geometry *uniform uniGeomPtr;
typedef uniform Material *uniform uniMaterialPtr;
typedef uniform Volume   *uniform uniVolumePtr;

export void *uniform DistributedModel_create(void *uniform cppE)
{
  uniform Model *uniform model = uniform new uniform Model;
  model->cppEquivalent     = cppE;
  model->embreeSceneHandle = NULL;
  model->geometry          = NULL;
  model->volumes           = NULL;
  return (void *uniform)model;
}

export void DistributedModel_init(void *uniform _model,
                                  void *uniform embreeDevice,
                                  uniform int32 numGeometries,
                                  uniform int32 numVolumes)
{
  uniform Model *uniform model = (uniform Model *uniform)_model;
  if (model->embreeSceneHandle)
    rtcDeleteScene(model->embreeSceneHandle);

  uniform RTCSceneFlags scene_flags = RTC_SCENE_STATIC | RTC_SCENE_HIGH_QUALITY;

  uniform RTCAlgorithmFlags traversal_flags =
      RTC_INTERSECT_UNIFORM | RTC_INTERSECT_VARYING;

  model->embreeSceneHandle = rtcDeviceNewScene((RTCDevice)embreeDevice,
                                               scene_flags,
                                               traversal_flags);

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

export void *uniform DistributedModel_getEmbreeSceneHandle(void *uniform _model)
{
  uniform Model *uniform model = (uniform Model *uniform)_model;
  return (void *uniform)model->embreeSceneHandle;
}

export void DistributedModel_setGeometry(void *uniform _model,
                                         uniform int32 geomID,
                                         void *uniform _geom)
{
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform Geometry *uniform geom = (uniform Geometry *uniform)_geom;
  model->geometry[geomID] = geom;
}

export void DistributedModel_setVolume(void *uniform pointer,
                                       uniform int32 index,
                                       void *uniform volume)
{
  uniform Model *uniform model = (uniform Model *uniform) pointer;
  model->volumes[index] = (uniform Volume *uniform) volume;
}
