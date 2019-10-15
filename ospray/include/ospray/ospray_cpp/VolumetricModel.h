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

#include "TransferFunction.h"
#include "Volume.h"

namespace ospray {
  namespace cpp {

    class VolumetricModel
        : public ManagedObject<OSPVolumetricModel, OSP_VOLUMETRIC_MODEL>
    {
     public:
      VolumetricModel(const Volume &geom);
      VolumetricModel(OSPVolume geom);
      VolumetricModel(const VolumetricModel &copy);
      VolumetricModel(OSPVolumetricModel existing = nullptr);
    };

    static_assert(sizeof(VolumetricModel) == sizeof(OSPVolumetricModel),
                  "cpp::VolumetricModel can't have data members!");

    // Inlined function definitions ///////////////////////////////////////////

    inline VolumetricModel::VolumetricModel(const Volume &geom)
        : VolumetricModel(geom.handle())
    {
    }

    inline VolumetricModel::VolumetricModel(OSPVolume existing)
    {
      ospObject = ospNewVolumetricModel(existing);
    }

    inline VolumetricModel::VolumetricModel(const VolumetricModel &copy)
        : ManagedObject<OSPVolumetricModel, OSP_VOLUMETRIC_MODEL>(copy.handle())
    {
      ospRetain(copy.handle());
    }

    inline VolumetricModel::VolumetricModel(OSPVolumetricModel existing)
        : ManagedObject<OSPVolumetricModel, OSP_VOLUMETRIC_MODEL>(existing)
    {
    }

  }  // namespace cpp

  OSPTYPEFOR_SPECIALIZATION(cpp::VolumetricModel, OSP_VOLUMETRIC_MODEL);

}  // namespace ospray
