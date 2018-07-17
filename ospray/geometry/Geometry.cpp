// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "Geometry.h"
#include "common/Data.h"
#include "common/Util.h"
// ISPC exports
#include "Geometry_ispc.h"

namespace ospray {

  Geometry::Geometry()
  {
    managedObjectType = OSP_GEOMETRY;
  }

  void Geometry::setMaterial(Material *mat)
  {
    if (!mat) {
      postStatusMsg() << "#osp: warning - tried to set NULL material; ignoring"
                      << "#osp: warning. (note this means that object may not "
                      << " get any material at all!)";
      return;
    }

    OSPMaterial ospMat = (OSPMaterial)mat;
    setMaterialList(new Data(1, OSP_OBJECT, &ospMat));
    mat->refDec(); // copied into matListData
  }

  void Geometry::setMaterialList(Data *matListData)
  {
    if (!matListData || matListData->numItems == 0) {
      postStatusMsg() << "#osp: warning - tried to set NULL material list; ignoring"
                      << "#osp: warning. (note this means that object may not "
                      << " get any material at all!)";
      return;
    }

    materialListData = matListData;
    materialList = (Material**)materialListData->data;

    if (!getIE()) {
      postStatusMsg("#osp: warning: geometry does not have an "
                    "ispc equivalent!");
    }
    else {
      const int numMaterials = materialListData->numItems;
      ispcMaterialPtrs.resize(numMaterials);
      for (int i = 0; i < numMaterials; i++)
        ispcMaterialPtrs[i] = materialList[i]->getIE();

      ispc::Geometry_setMaterialList(this->getIE(), ispcMaterialPtrs.data());
    }
  }

  Material *Geometry::getMaterial() const
  {
    return materialList ? materialList[0] : nullptr;
  }

  std::string Geometry::toString() const
  {
    return "ospray::Geometry";
  }

  void Geometry::finalize(Model *)
  {
    Data *materialListDataPtr = getParamData("materialList");
    if (materialListDataPtr)
      setMaterialList(materialListDataPtr);
  }

  Geometry *Geometry::createInstance(const char *type)
  {
    return createInstanceHelper<Geometry, OSP_GEOMETRY>(type);
  }

} // ::ospray
