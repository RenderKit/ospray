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
    if (embreeSceneHandle)
      rtcReleaseScene(embreeSceneHandle);

    ispc::World_cleanup(getIE());
  }

  std::string World::toString() const
  {
    return "ospray::World";
  }

  void World::commit()
  {
    useEmbreeDynamicSceneFlag = getParam<int>("dynamicScene", 0);
    useEmbreeCompactSceneFlag = getParam<int>("compactMode", 0);
    useEmbreeRobustSceneFlag  = getParam<int>("robustMode", 0);

    postStatusMsg(2)
        << "=======================================================\n"
        << "Finalizing model, has " << geometry.size() << " geometries, "
        << volume.size() << " volumes, and " << geometryInstances.size()
        << " geometry instances";

    RTCDevice embreeDevice = (RTCDevice)ospray_getEmbreeDevice();

    int sceneFlags = 0;
    sceneFlags =
        sceneFlags | (useEmbreeDynamicSceneFlag ? RTC_SCENE_FLAG_DYNAMIC : 0);
    sceneFlags =
        sceneFlags | (useEmbreeCompactSceneFlag ? RTC_SCENE_FLAG_COMPACT : 0);
    sceneFlags =
        sceneFlags | (useEmbreeRobustSceneFlag ? RTC_SCENE_FLAG_ROBUST : 0);

    ispc::World_init(getIE(),
                     embreeDevice,
                     sceneFlags,
                     geometry.size(),
                     geometryInstances.size(),
                     volume.size());

    embreeSceneHandle = (RTCScene)ispc::World_getEmbreeSceneHandle(getIE());

    bounds = empty;

    for (size_t i = 0; i < geometry.size(); i++) {
      postStatusMsg(2)
          << "=======================================================\n"
          << "Finalizing geometry " << i;

      rtcAttachGeometry(embreeSceneHandle, geometry[i]->embreeGeometry);

      bounds.extend(geometry[i]->bounds);
      ispc::World_setGeometry(getIE(), i, geometry[i]->getIE());
    }

    for (size_t i = 0; i < geometryInstances.size(); i++) {
      postStatusMsg(2)
          << "=======================================================\n"
          << "Finalizing geometry instance " << i;

      auto &instance = *geometryInstances[i];
      rtcAttachGeometry(embreeSceneHandle, instance.embreeGeometryHandle());
      bounds.extend(instance.bounds());
      ispc::World_setGeometryInstance(getIE(), i, instance.getIE());
    }

    for (size_t i = 0; i < volume.size(); i++) {
      ispc::World_setVolume(getIE(), i, volume[i]->getIE());
      box3f volBounds = empty;
      ispc::Volume_getBoundingBox((ispc::box3f *)&volBounds,
                                  volume[i]->getIE());
      bounds.extend(volBounds);
    }

    ispc::World_setBounds(getIE(), (ispc::box3f *)&bounds);

    rtcCommitScene(embreeSceneHandle);
  }

}  // namespace ospray
