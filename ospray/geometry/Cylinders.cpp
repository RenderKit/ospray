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
#include "Cylinders.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "Cylinders_ispc.h"

namespace ospray {

  std::string Cylinders::toString() const
  {
    return "ospray::Cylinders";
  }

  void Cylinders::commit()
  {
    radius           = getParam1f("radius", 0.01f);
    bytesPerCylinder = getParam1i("bytes_per_cylinder", 6 * sizeof(float));
    offset_v0        = getParam1i("offset_v0", 0);
    offset_v1        = getParam1i("offset_v1", 3 * sizeof(float));
    offset_radius    = getParam1i("offset_radius", -1);
    cylinderData     = getParamData("cylinders");
    texcoordData     = getParamData("texcoord");

    if (cylinderData.ptr == nullptr || bytesPerCylinder == 0) {
      throw std::runtime_error(
          "#ospray:geometry/cylinders: no 'cylinders'"
          " data specified");
    }

    numCylinders = cylinderData->numBytes / bytesPerCylinder;

    postStatusMsg(2) << "#osp: creating 'cylinders' geometry, #cylinders = "
                     << numCylinders;
  }

  size_t Cylinders::numPrimitives() const
  {
    return numCylinders;
  }

  LiveGeometry Cylinders::createEmbreeGeometry()
  {
    LiveGeometry retval;

    retval.embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);
    retval.ispcEquivalent = ispc::Cylinders_create(this);

    ispc::CylindersGeometry_set(retval.ispcEquivalent,
                                retval.embreeGeometry,
                                cylinderData->data,
                                texcoordData ? texcoordData->data : nullptr,
                                numCylinders,
                                bytesPerCylinder,
                                radius,
                                offset_v0,
                                offset_v1,
                                offset_radius);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(Cylinders, cylinders);

}  // namespace ospray
