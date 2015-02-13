// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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
#include "ospray/common/Data.h"
#include "ospray/common/Model.h"
// ispc-generated files
#include "Cylinders_ispc.h"

namespace ospray {

  Cylinders::Cylinders()
  {
    this->ispcEquivalent = ispc::Cylinders_create(this);
  }

  void Cylinders::finalize(Model *model) 
  {
    radius            = getParam1f("radius",0.01f);
    materialID        = getParam1i("materialID",0);
    bytesPerCylinder  = getParam1i("bytes_per_cylinder",7*sizeof(float));
    offset_v0         = getParam1i("offset_v0",0);
    offset_v1         = getParam1i("offset_v1",3*sizeof(float));
    offset_radius     = getParam1i("offset_radius",6*sizeof(float));
    offset_materialID = getParam1i("offset_materialID",-1);
    data              = getParamData("cylinders",NULL);
    
    if (data.ptr == NULL || bytesPerCylinder == 0) 
      throw std::runtime_error("#ospray:geometry/cylinders: no 'cylinders' data specified");
    numCylinders = data->numBytes / bytesPerCylinder;
    std::cout << "#osp: creating 'cylinders' geometry, #cylinders = " << numCylinders << std::endl;
    
    ispc::CylindersGeometry_set(getIE(),model->getIE(),
                                data->data,numCylinders,bytesPerCylinder,
                                radius,materialID,
                                offset_v0,offset_v1,offset_radius,offset_materialID);
  }


  OSP_REGISTER_GEOMETRY(Cylinders,cylinders);

} // ::ospray
