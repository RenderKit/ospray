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

  std::string Boxes::toString() const
  {
    return "ospray::Boxes";
  }

  void Boxes::commit()
  {
    boxData = getParamData("box");

    if (!boxData)
      throw std::runtime_error("no box data provided to Boxes geometry!");

    if (boxData->type == OSP_BOX3F)
      numBoxes = boxData->numItems;
    else if (boxData->type == OSP_VEC3F)
      numBoxes = boxData->numItems / 2;
    else if (boxData->type == OSP_FLOAT)
      numBoxes = boxData->numItems / 6;
    else
      throw std::runtime_error("unable to use element type for box geometry!");
  }

  size_t Boxes::numPrimitives() const
  {
    return numBoxes;
  }

  LiveGeometry Boxes::createEmbreeGeometry()
  {
    LiveGeometry retval;

    retval.embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);
    retval.ispcEquivalent = ispc::Boxes_create(this);

    ispc::Boxes_set(
        retval.ispcEquivalent, retval.embreeGeometry, numBoxes, boxData->data);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(Boxes, box);
  OSP_REGISTER_GEOMETRY(Boxes, boxes);

}  // namespace ospray
