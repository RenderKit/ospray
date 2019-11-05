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
#include "Mesh.h"
#include "../include/ospray/ospray.h"
#include "common/World.h"
// ispc exports
#include <cmath>
#include "Mesh_ispc.h"

namespace ospray {

  std::string Mesh::toString() const
  {
    return "ospray::Mesh";
  }

  void Mesh::commit()
  {
    vertexData = getParamDataT<vec3f>("vertex.position", true);
    normalData = getParamDataT<vec3f>("vertex.normal");
    colorData = getParam<Data *>("vertex.color");
    texcoordData = getParamDataT<vec2f>("vertex.texcoord");
    indexData = getParam<Data *>("index");
    postCreationInfo(vertexData->size());
  }

  size_t Mesh::numPrimitives() const
  {
    return indexData ? indexData->size() : 0;
  }

  LiveGeometry Mesh::createEmbreeGeometry()
  {
    RTCGeometry embreeGeo;
    if (indexData->type == OSP_VEC3UI) {
        embreeGeo = rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_TRIANGLE);
        rtcSetSharedGeometryBuffer(embreeGeo,
            RTC_BUFFER_TYPE_INDEX,
            0,
            RTC_FORMAT_UINT3,
            indexData->data(),
            0,
            sizeof(vec3ui),
            indexData->size());
    } else {
        embreeGeo = rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_QUAD);
        rtcSetSharedGeometryBuffer(embreeGeo,
            RTC_BUFFER_TYPE_INDEX,
            0,
            RTC_FORMAT_UINT4,
            indexData->data(),
            0,
            sizeof(vec4ui),
            indexData->size());
    }
    setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_VERTEX, vertexData);
    rtcCommitGeometry(embreeGeo);

    LiveGeometry retval;
    retval.embreeGeometry = embreeGeo;

    retval.ispcEquivalent = ispc::Mesh_create(this);

    ispc::Mesh_set(retval.ispcEquivalent,
        ispc(indexData),
        ispc(vertexData),
        ispc(normalData),
        ispc(colorData),
        ispc(texcoordData),
        colorData && colorData->type == OSP_VEC4F,
        indexData->type == OSP_VEC3UI);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(Mesh, quads);
  OSP_REGISTER_GEOMETRY(Mesh, triangles);

}  // namespace ospray
