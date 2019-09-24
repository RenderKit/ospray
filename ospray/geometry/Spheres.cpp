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
#include "Spheres.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "Spheres_ispc.h"

namespace ospray {

  std::string Spheres::toString() const
  {
    return "ospray::Spheres";
  }

  void Spheres::commit()
  {
    radius = getParam<float>("radius", 0.01f);
    vertexData = getParamDataT<vec3f>("sphere.position", true);
    radiusData = getParamDataT<float>("sphere.radius");
    texcoordData = getParamDataT<vec2f>("sphere.texcoord");

    postCreationInfo();
  }

  size_t Spheres::numPrimitives() const
  {
    return vertexData ? vertexData->size() : 0;
  }

  LiveGeometry Spheres::createEmbreeGeometry()
  {
    LiveGeometry retval;

    retval.ispcEquivalent = ispc::Spheres_create(this);
    retval.embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);

    ispc::SpheresGeometry_set(retval.ispcEquivalent,
        retval.embreeGeometry,
        ispc(vertexData),
        ispc(radiusData),
        ispc(texcoordData),
        radius);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(Spheres, spheres);

}  // namespace ospray
