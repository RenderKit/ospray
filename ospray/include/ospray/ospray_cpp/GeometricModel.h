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

#include "Geometry.h"
#include "Material.h"

namespace ospray {
  namespace cpp {

    class GeometricModel
        : public ManagedObject<OSPGeometricModel, OSP_GEOMETRIC_MODEL>
    {
     public:
      GeometricModel(const Geometry &geom);
      GeometricModel(OSPGeometry geom);
      GeometricModel(const GeometricModel &copy);
      GeometricModel(OSPGeometricModel existing = nullptr);
    };

    static_assert(sizeof(GeometricModel) == sizeof(OSPGeometricModel),
                  "cpp::GeometricModel can't have data members!");

    // Inlined function definitions ///////////////////////////////////////////

    inline GeometricModel::GeometricModel(const Geometry &geom)
        : GeometricModel(geom.handle())
    {
    }

    inline GeometricModel::GeometricModel(OSPGeometry existing)
    {
      ospObject = ospNewGeometricModel(existing);
    }

    inline GeometricModel::GeometricModel(const GeometricModel &copy)
        : ManagedObject<OSPGeometricModel, OSP_GEOMETRIC_MODEL>(copy.handle())
    {
      ospRetain(copy.handle());
    }

    inline GeometricModel::GeometricModel(OSPGeometricModel existing)
        : ManagedObject<OSPGeometricModel, OSP_GEOMETRIC_MODEL>(existing)
    {
    }

  }  // namespace cpp

  OSPTYPEFOR_SPECIALIZATION(cpp::GeometricModel, OSP_GEOMETRIC_MODEL);

}  // namespace ospray
