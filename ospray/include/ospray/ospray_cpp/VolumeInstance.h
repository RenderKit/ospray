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

#include <ospray/ospray_cpp/Volume.h>
#include <ospray/ospray_cpp/TransferFunction.h>

namespace ospray {
  namespace cpp {

    class VolumeInstance : public ManagedObject_T<OSPVolumeInstance>
    {
     public:
      VolumeInstance(const Volume &geom);
      VolumeInstance(OSPVolume geom);

      VolumeInstance(const VolumeInstance &copy);
      VolumeInstance(OSPVolumeInstance existing);

      void setTransferFunction(TransferFunction &m) const;
      void setTransferFunction(OSPTransferFunction m) const;
    };

    // Inlined function definitions ///////////////////////////////////////////

    inline VolumeInstance::VolumeInstance(const Volume &geom)
        : VolumeInstance(geom.handle())
    {
    }

    inline VolumeInstance::VolumeInstance(OSPVolume existing)
    {
      ospObject = ospNewVolumeInstance(existing);
    }

    inline VolumeInstance::VolumeInstance(const VolumeInstance &copy)
        : ManagedObject_T<OSPVolumeInstance>(copy.handle())
    {
    }

    inline VolumeInstance::VolumeInstance(OSPVolumeInstance existing)
        : ManagedObject_T<OSPVolumeInstance>(existing)
    {
    }

    inline void VolumeInstance::setTransferFunction(TransferFunction &m) const
    {
      setTransferFunction(m.handle());
    }

    inline void VolumeInstance::setTransferFunction(OSPTransferFunction m) const
    {
      set("transferFunction", m);
    }

  }  // namespace cpp
}  // namespace ospray
