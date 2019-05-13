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

#include "common/Data.h"
#include "Volume.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE VolumeInstance : public ManagedObject
  {
    VolumeInstance(Volume *geometry);
    virtual ~VolumeInstance() override;
    virtual std::string toString() const override;

    virtual void commit() override;

    RTCGeometry embreeGeometryHandle() const;

    box3f bounds() const;

    AffineSpace3f xfm() const;

   private:
    // Data members //

    // Volume information
    box3f instanceBounds;
    AffineSpace3f instanceXfm;
    Ref<Volume> instancedVolume;

    // Embree information
    RTCScene embreeSceneHandle{nullptr};
    RTCGeometry embreeInstanceGeometry{nullptr};
    RTCGeometry lastEmbreeInstanceGeometryHandle{nullptr}; // to detect updates
    int embreeID{-1};
  };

}  // namespace ospray
