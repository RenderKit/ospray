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

// ospray
#include "GeometricModel.h"
// ispc exports
#include "GeometricModel_ispc.h"
#include "Geometry_ispc.h"

namespace ospray {

  GeometricModel::GeometricModel(Geometry *_geometry)
  {
    managedObjectType = OSP_GEOMETRIC_MODEL;
    geom = _geometry;

    this->ispcEquivalent = ispc::GeometricModel_create(this);

    setMaterialList(nullptr);
  }

  std::string GeometricModel::toString() const
  {
    return "ospray::GeometricModel";
  }

  void GeometricModel::setMaterial()
  {
    auto *data = new Data(&material.ptr, OSP_MATERIAL, vec3ui(1), vec3l(0));
    setMaterialList(&(data->as<Material *>()));
    data->refDec();
  }

  void GeometricModel::setMaterialList(const DataT<Material *> *matListData)
  {
    if (!matListData || matListData->size() == 0) {
      ispc::GeometricModel_setMaterialList(this->getIE(), 0, nullptr);
      return;
    }

    materialListData = matListData;
    materialList = materialListData->data();
    const int numMaterials = materialListData->size();
    ispcMaterialPtrs.resize(numMaterials);
    for (int i = 0; i < numMaterials; i++)
      ispcMaterialPtrs[i] = materialList[i]->getIE();

    ispc::GeometricModel_setMaterialList(
        this->getIE(), numMaterials, ispcMaterialPtrs.data());
  }

  void GeometricModel::commit()
  {
    colorData = getParamDataT<vec4f>("prim.color");

    if (colorData && colorData->size() != geom->numPrimitives()) {
      postStatusMsg(1)
          << toString()
          << " number of colors does not match number of primitives, ignoring 'prim.color'";
      colorData = nullptr;
    }

    prim_materialIDData = getParamDataT<uint32_t>("prim.materialID");
    auto materialListDataPtr = getParamDataT<Material *>("materialList");

    if (prim_materialIDData
        && prim_materialIDData->size() != geom->numPrimitives()) {
      postStatusMsg(1)
          << toString()
          << " number of primitive material IDs does not match number of primitives, ignoring 'prim.materialID'";
      prim_materialIDData = nullptr;
    }

    material = (Material *)getParamObject("material");

    if (materialListDataPtr && prim_materialIDData)
      setMaterialList(materialListDataPtr);
    else if (material)
      setMaterial();

    ispc::GeometricModel_set(getIE(),
        colorData ? colorData->data() : nullptr,
        prim_materialIDData ? prim_materialIDData->data() : nullptr);
  }

  void GeometricModel::setGeomIE(void *geomIE, int geomID)
  {
    ispc::Geometry_set_geomID(geomIE, geomID);
    ispc::GeometricModel_setGeomIE(getIE(), geomIE);
  }

  OSPTYPEFOR_DEFINITION(GeometricModel *);

}  // namespace ospray
