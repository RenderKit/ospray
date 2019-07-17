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
    geom = _geometry;

    this->ispcEquivalent = ispc::GeometricModel_create(this);

    setMaterialList(nullptr);
  }

  std::string GeometricModel::toString() const
  {
    return "ospray::GeometricModel";
  }

  void GeometricModel::setMaterial(Material *mat)
  {
    OSPMaterial ospMat = (OSPMaterial)mat;
    auto *data         = new Data(1, OSP_OBJECT, &ospMat);
    setMaterialList(data);
    data->refDec();
  }

  void GeometricModel::setMaterialList(Data *matListData)
  {
    if (!matListData || matListData->numItems == 0) {
      ispc::GeometricModel_setMaterialList(this->getIE(), 0, nullptr);
      return;
    }

    materialListData = matListData;
    materialList     = (Material **)materialListData->data;

    const int numMaterials = materialListData->numItems;
    ispcMaterialPtrs.resize(numMaterials);
    for (int i = 0; i < numMaterials; i++)
      ispcMaterialPtrs[i] = materialList[i]->getIE();

    ispc::GeometricModel_setMaterialList(
        this->getIE(), numMaterials, ispcMaterialPtrs.data());
  }

  void GeometricModel::commit()
  {
    colorData = getParamData("prim.color");

    if (colorData && colorData->numItems != geom->numPrimitives()) {
      throw std::runtime_error(
          "number of colors does not match number of primitives!");
    }

    prim_materialIDData       = getParamData("prim.materialID");
    Data *materialListDataPtr = getParamData("materialList");

    if (prim_materialIDData &&
        prim_materialIDData->numItems != geom->numPrimitives()) {
      throw std::runtime_error(
          "number of prim.materialID does not match number of primitives!");
    }

    material = (Material *)getParamObject("material");

    if (materialListDataPtr && prim_materialIDData)
      setMaterialList(materialListDataPtr);
    else if (material)
      setMaterial(material.ptr);

    ispc::GeometricModel_set(
        getIE(),
        colorData ? colorData->data : nullptr,
        prim_materialIDData ? prim_materialIDData->data : nullptr);
  }

  void GeometricModel::setGeomIE(void *geomIE, int geomID)
  {
    ispc::Geometry_set_geomID(geomIE, geomID);
    ispc::GeometricModel_setGeomIE(getIE(), geomIE);
  }

}  // namespace ospray
