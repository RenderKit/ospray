// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
      rtcDeleteScene(embreeSceneHandle);

    ispc::Model_cleanup(getIE());
  }

  std::string Model::toString() const
  {
    return "ospray::Model";
  }

  void Model::commit()
  {
    postStatusMsg(2)
        << "=======================================================\n"
        << "Finalizing model, has " << geometry.size()
        << " geometries and " << volume.size() << " volumes";

    RTCDevice embreeDevice = (RTCDevice)ospray_getEmbreeDevice();

    ispc::Model_init(getIE(), embreeDevice, geometry.size(), volume.size());

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

    for (size_t i=0; i<volume.size(); i++)
      ispc::Model_setVolume(getIE(), i, volume[i]->getIE());

    rtcCommit(embreeSceneHandle);
  }

} // ::ospray
