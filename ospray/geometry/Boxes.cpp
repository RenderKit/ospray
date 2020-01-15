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
#include "Boxes.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "Boxes_ispc.h"

namespace ospray {

  Boxes::Boxes()
  {
    embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);
    ispcEquivalent = ispc::Boxes_create(this);
  }

  std::string Boxes::toString() const
  {
    return "ospray::Boxes";
  }

  void Boxes::commit()
  {
    boxData = getParamDataT<box3f>("box", true);

    ispc::Boxes_set(getIE(), embreeGeometry, ispc(boxData));

    postCreationInfo();
  }

  size_t Boxes::numPrimitives() const
  {
    return boxData ? boxData->size() : 0;
  }

  OSP_REGISTER_GEOMETRY(Boxes, box);

}  // namespace ospray
