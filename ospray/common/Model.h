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
#include "geometry/Geometry.h"
#include "volume/Volume.h"

// stl
#include <vector>

// embree
#include "embree3/rtcore.h"

namespace ospray {

  /*! \brief Base Abstraction for an OSPRay 'Model' entity

    A 'model' is the generalization of a 'scene' in embree: it is a
    collection of geometries and volumes that one can trace rays
    against, and that one can afterwards 'query' for certain
    properties (like the shading normal or material for a given
    ray/model intersection) */
  struct OSPRAY_SDK_INTERFACE Model : public ManagedObject
  {
    Model();
    virtual ~Model() override;

    //! \brief common function to help printf-debugging
    virtual std::string toString() const override;
    virtual void commit() override;

    // Data members //

    using GeometryVector = std::vector<Ref<Geometry>>;
    using VolumeVector   = std::vector<Ref<Volume>>;

    //! \brief vector of all geometries used in this model
    GeometryVector geometry;
    //! \brief vector of all volumes used in this model
    VolumeVector volume;

    //! \brief the embree scene handle for this geometry
    RTCScene embreeSceneHandle {nullptr};
    box3f bounds;

    bool useEmbreeDynamicSceneFlag{true};
    bool useEmbreeCompactSceneFlag{false};
    bool useEmbreeRobustSceneFlag{false};
  };

} // ::ospray
