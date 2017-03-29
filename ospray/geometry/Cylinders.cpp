// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
  }

  std::string Cylinders::toString() const
  {
    return "ospray::Cylinders";
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

    if (cylinderData.ptr == nullptr || bytesPerCylinder == 0) {
      throw std::runtime_error("#ospray:geometry/cylinders: no 'cylinders'"
                               " data specified");
    }
    numCylinders = cylinderData->numBytes / bytesPerCylinder;
    std::stringstream msg;
    msg << "#osp: creating 'cylinders' geometry, #cylinders = "
        << numCylinders << std::endl;
    postErrorMsg(msg, 2);

    if (_materialList) {
      free(_materialList);
      _materialList = nullptr;
    }

    if (materialList) {
      void **ispcMaterials =
          (void**) malloc(sizeof(void*) * materialList->numItems);
      for (uint32_t i = 0; i < materialList->numItems; i++) {
        Material *m = ((Material**)materialList->data)[i];
        ispcMaterials[i] = m?m->getIE():nullptr;
      }
      _materialList = (void*)ispcMaterials;
    }

    const char* cylinderPtr = (const char*)cylinderData->data;
    bounds = empty;
    for (uint32_t i = 0; i < numCylinders; i++, cylinderPtr += bytesPerCylinder) {
      const float r = offset_radius < 0 ? radius : *(float*)(cylinderPtr + offset_radius);
      const vec3f v0 = *(vec3f*)(cylinderPtr + offset_v0);
      const vec3f v1 = *(vec3f*)(cylinderPtr + offset_v1);
      bounds.extend(box3f(v0 - r, v0 + r));
      bounds.extend(box3f(v1 - r, v1 + r));
    }

    ispc::CylindersGeometry_set(getIE(),model->getIE(),
                                cylinderData->data,_materialList,
                                colorData?(ispc::vec4f*)colorData->data:nullptr,
                                numCylinders,bytesPerCylinder,
                                radius,materialID,
                                offset_v0,offset_v1,
                                offset_radius,
                                offset_materialID,offset_colorID);
  }


  OSP_REGISTER_GEOMETRY(Cylinders,cylinders);

} // ::ospray
