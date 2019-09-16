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
#include "QuadMesh.h"
#include "../include/ospray/ospray.h"
#include "common/World.h"
// ispc exports
#include <cmath>
#include "QuadMesh_ispc.h"

namespace ospray {

  std::string QuadMesh::toString() const
  {
    return "ospray::QuadMesh";
  }

  void QuadMesh::commit()
  {
    vertexData = getParamDataT<vec3f>("vertex.position", true);
    normalData = getParamDataT<vec3f>("vertex.normal");
    colorData    = getParamData("vertex.color");
    texcoordData = getParamDataT<vec2f>("vertex.texcoord");
    indexData = getParamDataT<vec4ui>("index", true);

    postCreationInfo(vertexData->size());
  }

  size_t QuadMesh::numPrimitives() const
  {
    return indexData ? indexData->size() : 0;
  }

  LiveGeometry QuadMesh::createEmbreeGeometry()
  {
    auto embreeGeo =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_QUAD);

    setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_VERTEX, vertexData);
    setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_INDEX, indexData);

    rtcCommitGeometry(embreeGeo);

    LiveGeometry retval;
    retval.embreeGeometry = embreeGeo;

    retval.ispcEquivalent = ispc::QuadMesh_create(this);

    ispc::QuadMesh_set(retval.ispcEquivalent,
        ispc(indexData),
        ispc(vertexData),
        ispc(normalData),
        ispc(colorData),
        ispc(texcoordData),
        colorData && colorData->type == OSP_VEC4F);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(QuadMesh, quads);

}  // namespace ospray
