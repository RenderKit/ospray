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

#pragma once

#include "api/ISPCDevice.h"
#include "common/Managed.h"
// embree
#include "Volume_ispc.h"
#include "embree3/rtcore.h"

#include "openvkl/volume.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE Volume : public ManagedObject
  {
    Volume(const std::string &vklType);
    ~Volume() override;

    std::string toString() const override;

    void commit() override;

   private:
    void handleParams();

    void createEmbreeGeometry();

    // Friends //

    friend struct Isosurfaces;
    friend struct VolumetricModel;

    // Data //

    RTCGeometry embreeGeometry{nullptr};
    VKLVolume vklVolume{nullptr};

    box3f bounds{empty};

    std::string vklType;
  };

  OSPTYPEFOR_SPECIALIZATION(Volume *, OSP_VOLUME);

}  // namespace ospray
