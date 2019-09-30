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
  }

  std::string GeometricModel::toString() const
  {
    return "ospray::GeometricModel";
  }

  void GeometricModel::commit()
  {
    materialData = getParamDataT<Material *>("material");
    colorData = getParamDataT<vec4f>("color");
    indexData = getParamDataT<uint8_t>("index");

    size_t maxItems = geom->numPrimitives();
    size_t minItems = 0; // without index, a single material / color is OK
    if (indexData && indexData->size() < maxItems) {
      postStatusMsg(1)
          << toString()
          << " not enough 'index' elements for geometry, clamping";
    }
    if (indexData) {
      maxItems = 256; // conservative, should actually go over the index
      minItems = 1;
    }

    if (materialData && materialData->size() > minItems
        && materialData->size() < maxItems) {
      postStatusMsg(1)
          << toString()
          << " potentially not enough 'material' elements for geometry, clamping";
    }

    if (colorData && colorData->size() > minItems
        && colorData->size() < maxItems) {
      postStatusMsg(1)
          << toString()
          << " potentially not enough 'color' elements for geometry, clamping";
    }

    if (materialData)
      ispcMaterialPtrs = createArrayOfIE(*materialData);
    else
      ispcMaterialPtrs.clear();

    ispc::GeometricModel_set(getIE(),
        ispc(colorData),
        ispc(indexData),
        ispcMaterialPtrs.size(),
        ispcMaterialPtrs.data());
  }

  void GeometricModel::setGeomIE(void *geomIE, int geomID)
  {
    ispc::Geometry_set_geomID(geomIE, geomID);
    ispc::GeometricModel_setGeomIE(getIE(), geomIE);
  }

  OSPTYPEFOR_DEFINITION(GeometricModel *);

}  // namespace ospray
