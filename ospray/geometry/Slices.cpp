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

#undef NDEBUG

// ospray
#include "Slices.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "Slices_ispc.h"

namespace ospray {

  std::string Slices::toString() const
  {
    return "ospray::Slices";
  }

  void Slices::commit()
  {
    planesData = getParamData("plane", nullptr);
    volume     = (VolumetricModel *)getParamObject("volume", nullptr);

    numPlanes = planesData->numItems;
    planes    = (vec4f *)planesData->data;
  }

  size_t Slices::numPrimitives() const
  {
    return numPlanes;
  }

  LiveGeometry Slices::createEmbreeGeometry()
  {
    LiveGeometry retval;

    retval.ispcEquivalent = ispc::Slices_create(this);
    retval.embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);

    ispc::Slices_set(retval.ispcEquivalent,
                     retval.embreeGeometry,
                     numPlanes,
                     (ispc::vec4f *)planes,
                     volume->getIE());

    return retval;
  }

  OSP_REGISTER_GEOMETRY(Slices, slices);

}  // namespace ospray
