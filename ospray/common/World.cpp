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

// ospray
#include "World.h"
#include "api/ISPCDevice.h"
// ispc exports
#include "Volume_ispc.h"
#include "World_ispc.h"

namespace ospray {

  extern "C" void *ospray_getEmbreeDevice()
  {
    return api::ISPCDevice::embreeDevice;
  }

  World::World()
  {
    managedObjectType    = OSP_WORLD;
    this->ispcEquivalent = ispc::World_create(this);
  }

  World::~World()
  {
    if (embreeSceneHandleGeometries)
      rtcReleaseScene(embreeSceneHandleGeometries);
    if (embreeSceneHandleVolumes)
      rtcReleaseScene(embreeSceneHandleVolumes);

    ispc::World_cleanup(getIE());
  }

  std::string World::toString() const
  {
    return "ospray::World";
  }

  void World::commit()
  {
    useEmbreeDynamicSceneFlag = getParam<bool>("dynamicScene", 0);
    useEmbreeCompactSceneFlag = getParam<bool>("compactMode", 0);
    useEmbreeRobustSceneFlag  = getParam<bool>("robustMode", 0);

    geometryInstances = (Data*)getParamObject("geometries");
    volumeInstances   = (Data*)getParamObject("volumes");

    size_t numGeometries = geometryInstances ? geometryInstances->size() : 0;
    size_t numVolumes    = volumeInstances ? volumeInstances->size() : 0;

    postStatusMsg(2)
        << "=======================================================\n"
        << "Finalizing model, which has " << numGeometries << " geometries and "
        << numVolumes << " volumes";

    RTCDevice embreeDevice = (RTCDevice)ospray_getEmbreeDevice();

    int sceneFlags = 0;
    sceneFlags =
        sceneFlags | (useEmbreeDynamicSceneFlag ? RTC_SCENE_FLAG_DYNAMIC : 0);
    sceneFlags =
        sceneFlags | (useEmbreeCompactSceneFlag ? RTC_SCENE_FLAG_COMPACT : 0);
    sceneFlags =
        sceneFlags | (useEmbreeRobustSceneFlag ? RTC_SCENE_FLAG_ROBUST : 0);

    ispc::World_init(
        getIE(), embreeDevice, sceneFlags, numGeometries, numVolumes);

    embreeSceneHandleGeometries =
        (RTCScene)ispc::World_getEmbreeSceneHandleGeometries(getIE());
    embreeSceneHandleVolumes =
        (RTCScene)ispc::World_getEmbreeSceneHandleVolumes(getIE());

    bounds = empty;

    for (size_t i = 0; i < numGeometries; i++) {
      postStatusMsg(2)
          << "=======================================================\n"
          << "Finalizing geometry instance " << i;

      auto &instance = *geometryInstances->at<GeometryInstance *>(i);
      rtcAttachGeometry(embreeSceneHandleGeometries,
                        instance.embreeGeometryHandle());
      bounds.extend(instance.bounds());
      ispc::World_setGeometryInstance(getIE(), i, instance.getIE());
    }

    for (size_t i = 0; i < numVolumes; i++) {
      postStatusMsg(2)
          << "=======================================================\n"
          << "Finalizing volume instance " << i;

      auto &instance = *volumeInstances->at<VolumeInstance *>(i);
      rtcAttachGeometry(embreeSceneHandleVolumes,
                        instance.embreeGeometryHandle());
      bounds.extend(instance.bounds());
      ispc::World_setVolumeInstance(getIE(), i, instance.getIE());
    }

    ispc::World_setBounds(getIE(), (ispc::box3f *)&bounds);

    rtcCommitScene(embreeSceneHandleGeometries);
    rtcCommitScene(embreeSceneHandleVolumes);
  }

}  // namespace ospray
