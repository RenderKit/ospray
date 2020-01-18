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

// ospray stuff
#include "../geometry/GeometricModel.h"
#include "../volume/VolumetricModel.h"
#include "./Data.h"
#include "./Managed.h"
// stl
#include <vector>
// embree
#include "embree3/rtcore.h"
// ospcommon
#include "ospcommon/utility/Optional.h"

namespace ospray {

  using OptionalScene = utility::Optional<RTCScene>;

  struct OSPRAY_SDK_INTERFACE Group : public ManagedObject
  {
    Group();
    ~Group() override;

    std::string toString() const override;
    void commit() override;

    box3f getBounds() const override;

    OptionalScene embreeGeometryScene();
    OptionalScene embreeVolumeScene();

    // Data members //

    Ref<const DataT<GeometricModel *>> geometricModels;
    std::vector<void *> geometryIEs;  // NOTE: needs to be freed!
    std::vector<void *> geometricModelIEs;

    Ref<const DataT<VolumetricModel *>> volumetricModels;
    std::vector<void *> volumeIEs;  // NOTE: needs to be freed!
    std::vector<void *> volumetricModelIEs;

    RTCScene sceneGeometries{nullptr};
    RTCScene sceneVolumes{nullptr};
  };

  OSPTYPEFOR_SPECIALIZATION(Group *, OSP_GROUP);

  // Inlined members /////////////////////////////////////////////////////////

  inline OptionalScene Group::embreeGeometryScene()
  {
    return sceneGeometries ? sceneGeometries : OptionalScene();
  }

  inline OptionalScene Group::embreeVolumeScene()
  {
    return sceneVolumes ? sceneVolumes : OptionalScene();
  }

}  // namespace ospray
