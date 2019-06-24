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
#include "geometry/GeometricModel.h"
#include "volume/VolumetricModel.h"
// embree
#include "embree3/rtcore.h"
// ospcommon
#include "ospcommon/utility/Optional.h"

namespace ospray {

  using OptionalScene = utility::Optional<RTCScene>;

  struct OSPRAY_SDK_INTERFACE Instance : public ManagedObject
  {
    Instance();
    ~Instance() override;

    std::string toString() const override;

    void commit() override;

    affine3f xfm();

    OptionalScene embreeGeometryScene();
    OptionalScene embreeVolumeScene();

   private:
    // Geometry information
    affine3f instanceXfm;
    affine3f rcpXfm;

    Ref<Data> geometricModels;
    std::vector<void *> geometricModelIEs;

    Ref<Data> volumetricModels;
    std::vector<void *> volumetricModelIEs;

    //! \brief the embree scene handle for this geometry
    RTCScene sceneGeometries{nullptr};
    RTCScene sceneVolumes{nullptr};

    friend struct PathTracer;  // TODO: fix this!
    friend struct Renderer;
  };

  // Inlined definitions //////////////////////////////////////////////////////

  inline affine3f Instance::xfm()
  {
    return instanceXfm;
  }

  inline OptionalScene Instance::embreeGeometryScene()
  {
    return sceneGeometries ? sceneGeometries : OptionalScene();
  }

  inline OptionalScene Instance::embreeVolumeScene()
  {
    return sceneVolumes ? sceneVolumes : OptionalScene();
  }

}  // namespace ospray
