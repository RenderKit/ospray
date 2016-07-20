// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#undef NDEBUG

// ospray
#include "Cylinders.h"
#include "common/Data.h"
#include "common/Model.h"
// ispc-generated files
#include "Cylinders_ispc.h"

namespace ospray {

  Cylinders::Cylinders()
  {
    this->ispcEquivalent = ispc::Cylinders_create(this);
    _materialList = NULL;
  }

  void Cylinders::finalize(Model *model)
  {
    radius            = getParam1f("radius",0.01f);
    materialID        = getParam1i("materialID",0);
    bytesPerCylinder  = getParam1i("bytes_per_cylinder",6*sizeof(float));
    offset_v0         = getParam1i("offset_v0",0);
    offset_v1         = getParam1i("offset_v1",3*sizeof(float));
    offset_radius     = getParam1i("offset_radius",-1);
    offset_materialID = getParam1i("offset_materialID",-1);
    offset_colorID    = getParam1i("offset_colorID",-1);
    cylinderData      = getParamData("cylinders");
    materialList      = getParamData("materialList");
    colorData         = getParamData("color");

    if (cylinderData.ptr == NULL || bytesPerCylinder == 0)
      throw std::runtime_error("#ospray:geometry/cylinders: no 'cylinders' data specified");
    numCylinders = cylinderData->numBytes / bytesPerCylinder;
    std::cout << "#osp: creating 'cylinders' geometry, #cylinders = " << numCylinders << std::endl;

    if (_materialList) {
      free(_materialList);
      _materialList = NULL;
    }

    if (materialList) {
      void **ispcMaterials = (void**) malloc(sizeof(void*) * materialList->numItems);
      for (int i=0;i<materialList->numItems;i++) {
        Material *m = ((Material**)materialList->data)[i];
        ispcMaterials[i] = m?m->getIE():NULL;
      }
      _materialList = (void*)ispcMaterials;
    }

    ispc::CylindersGeometry_set(getIE(),model->getIE(),
                                cylinderData->data,_materialList,
                                colorData?(ispc::vec4f*)colorData->data:NULL,
                                numCylinders,bytesPerCylinder,
                                radius,materialID,
                                offset_v0,offset_v1,
                                offset_radius,
                                offset_materialID,offset_colorID);
  }


  OSP_REGISTER_GEOMETRY(Cylinders,cylinders);

} // ::ospray
