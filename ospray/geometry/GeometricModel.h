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
#include "common/Data.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE GeometricModel : public ManagedObject
  {
    GeometricModel(Geometry *geometry);
    ~GeometricModel() override = default;

    std::string toString() const override;

    void commit() override;

    Geometry &geometry();

    void setGeomIE(void *geomIE, int geomID);

   private:
    // Helper functions //

    void setMaterial(Material *mat);
    void setMaterialList(Data *matListData);

    // Data //

    Ref<Geometry> geom;

    Ref<Data> prim_materialIDData; /*!< data array for per-prim material ID
                                      (uint32) */
    Ref<Data> colorData;
    Material **materialList{nullptr};  //!< per-primitive material list
    Ref<Material> material;
    Ref<Data> materialListData;  //!< data array for per-prim materials
    std::vector<void *>
        ispcMaterialPtrs;  //!< pointers to ISPC equivalent materials

    friend struct PathTracer;  // TODO: fix this!
    friend struct Renderer;
  };

  // Inlined members //////////////////////////////////////////////////////////

  inline Geometry &GeometricModel::geometry()
  {
    return *geom;
  }

}  // namespace ospray
