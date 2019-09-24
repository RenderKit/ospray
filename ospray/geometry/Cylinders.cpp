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
    radius           = getParam<float>("radius", 0.01f);
    vertex0Data = getParamDataT<vec3f>("cylinder.position0", true);
    vertex1Data = getParamDataT<vec3f>("cylinder.position1", true);
    if (vertex0Data->size() != vertex1Data->size())
      throw std::runtime_error(toString()
          + ": arrays 'cylinder.position0' and 'cylinder.position1' need to be of same size.");
    radiusData = getParamDataT<float>("cylinder.radius");
    texcoord0Data = getParamDataT<vec2f>("cylinder.texcoord0");
    texcoord1Data = getParamDataT<vec2f>("cylinder.texcoord1");

    postCreationInfo();
  }

  size_t Cylinders::numPrimitives() const
  {
    return vertex0Data ? vertex0Data->size() : 0;
  }

  LiveGeometry Cylinders::createEmbreeGeometry()
  {
    LiveGeometry retval;

    retval.embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);
    retval.ispcEquivalent = ispc::Cylinders_create(this);

    ispc::CylindersGeometry_set(retval.ispcEquivalent,
        retval.embreeGeometry,
        ispc(vertex0Data),
        ispc(vertex1Data),
        ispc(radiusData),
        ispc(texcoord0Data),
        ispc(texcoord1Data),
        radius);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(Cylinders, cylinders);

}  // namespace ospray
