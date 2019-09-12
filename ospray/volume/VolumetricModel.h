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

#include "Volume.h"
#include "common/Data.h"
#include "common/Material.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE VolumetricModel : public ManagedObject
  {
    VolumetricModel(Volume *geometry);
    ~VolumetricModel() override = default;
    std::string toString() const override;

    void commit() override;

    RTCGeometry embreeGeometryHandle() const;

    box3f bounds() const;

    void setGeomID(int geomID);

   private:
    box3f volumeBounds;
    Ref<Volume> volume;
  };

  OSPTYPEFOR_SPECIALIZATION(VolumetricModel *, OSP_VOLUMETRIC_MODEL);

}  // namespace ospray
