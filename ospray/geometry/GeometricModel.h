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
    void setMaterial();
    void setMaterialList(const DataT<Material *> *);

    Ref<Geometry> geom;
    Ref<const DataT<uint32_t>> prim_materialIDData;
    Ref<const DataT<vec4f>> colorData;
    Material **materialList{nullptr};  //!< per-primitive material list
    Ref<Material> material;
    Ref<const DataT<Material *>> materialListData;
    std::vector<void *> ispcMaterialPtrs;

    friend struct PathTracer;  // TODO: fix this!
    friend struct Renderer;
  };

  OSPTYPEFOR_SPECIALIZATION(GeometricModel *, OSP_GEOMETRIC_MODEL);

  // Inlined members //////////////////////////////////////////////////////////

  inline Geometry &GeometricModel::geometry()
  {
    return *geom;
  }

}  // namespace ospray
