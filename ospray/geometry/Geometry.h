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
#include "common/Data.h"
#include "common/Managed.h"
#include "common/Material.h"
#include "common/OSPCommon.h"
// embree
#include "embree3/rtcore.h"

namespace ospray {

  struct LiveGeometry
  {
    void *ispcEquivalent{nullptr};
    RTCGeometry embreeGeometry{nullptr};
  };

  struct OSPRAY_SDK_INTERFACE Geometry : public ManagedObject
  {
    Geometry();
    virtual ~Geometry() override = default;

    virtual std::string toString() const override;

    virtual size_t numPrimitives() const = 0;

    virtual LiveGeometry createEmbreeGeometry() = 0;

    // Object factory //

    static Geometry *createInstance(const char *type);
  };

#define OSP_REGISTER_GEOMETRY(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(                                      \
      ::ospray::Geometry, geometry, InternalClass, external_name)

}  // namespace ospray
