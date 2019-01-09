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
#include "api/ISPCDevice.h"
#include "Model.h"
// ispc exports
#include "Model_ispc.h"
#include "Volume_ispc.h"

namespace ospray {

  extern "C" void *ospray_getEmbreeDevice()
  {
    return api::ISPCDevice::embreeDevice;
  }

  Model::Model()
  {
    managedObjectType = OSP_MODEL;
    this->ispcEquivalent = ispc::Model_create(this);
  }

  Model::~Model()
  {
    if (embreeSceneHandle)
      rtcReleaseScene(embreeSceneHandle);

    ispc::Model_cleanup(getIE());
  }

  std::string Model::toString() const
  {
    return "ospray::Model";
  }

  void Model::commit()
  {
    useEmbreeDynamicSceneFlag = getParam<int>("dynamicScene", 0);
    useEmbreeCompactSceneFlag = getParam<int>("compactMode", 0);
    useEmbreeRobustSceneFlag = getParam<int>("robustMode", 0);

    postStatusMsg(2)
        << "=======================================================\n"
        << "Finalizing model, has " << geometry.size()
        << " geometries and " << volume.size() << " volumes";

    RTCDevice embreeDevice = (RTCDevice)ospray_getEmbreeDevice();

    int sceneFlags = 0;
    sceneFlags =
        sceneFlags | (useEmbreeDynamicSceneFlag ? RTC_SCENE_FLAG_DYNAMIC : 0);
    sceneFlags =
        sceneFlags | (useEmbreeCompactSceneFlag ? RTC_SCENE_FLAG_COMPACT : 0);
    sceneFlags =
        sceneFlags | (useEmbreeRobustSceneFlag ? RTC_SCENE_FLAG_ROBUST : 0);

    ispc::Model_init(getIE(),
                     embreeDevice,
                     sceneFlags,
                     geometry.size(),
                     volume.size());

    embreeSceneHandle = (RTCScene)ispc::Model_getEmbreeSceneHandle(getIE());

    bounds = empty;

    for (size_t i = 0; i < geometry.size(); i++) {
       postStatusMsg(2)
           << "=======================================================\n"
           << "Finalizing geometry " << i;

      geometry[i]->finalize(this);

      bounds.extend(geometry[i]->bounds);
      ispc::Model_setGeometry(getIE(), i, geometry[i]->getIE());
    }

    for (size_t i=0; i<volume.size(); i++) {
      ispc::Model_setVolume(getIE(), i, volume[i]->getIE());
      box3f volBounds = empty;
      ispc::Volume_getBoundingBox((ispc::box3f*)&volBounds, volume[i]->getIE());
      bounds.extend(volBounds);
    }

    ispc::Model_setBounds(getIE(), (ispc::box3f*)&bounds);

    rtcCommitScene(embreeSceneHandle);
  }

} // ::ospray
