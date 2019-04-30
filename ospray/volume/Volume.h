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
#include "embree3/rtcore.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE Volume : public ManagedObject
  {
    virtual ~Volume() override;

    virtual std::string toString() const override;

    static Volume *createInstance(const std::string &type);

    virtual void commit() override;

    virtual int setRegion(const void *source,
                          const vec3i &index,
                          const vec3i &count) = 0;

    box3f bounds{empty};

    RTCGeometry embreeGeometry{nullptr};

   private:
    void createEmbreeGeometry();
  };

#define OSP_REGISTER_VOLUME(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(::ospray::Volume, volume, InternalClass, external_name)

}  // namespace ospray
