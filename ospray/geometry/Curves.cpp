// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "Curves.h"
#include "common/Data.h"
#include "common/Model.h"
#include "ospcommon/utility/DataView.h"
// ispc-generated files
#include "Curves_ispc.h"

namespace ospray {

  Curves::Curves()
  {
    this->ispcEquivalent = ispc::Curves_create(this);
  }

  std::string Curves::toString() const
  {
    return "ospray::Curves";
  }

  void Curves::finalize(Model *model)
  {
    Geometry::finalize(model);

    vertexData = getParamData("vertex",nullptr);
    if (!vertexData)
      throw std::runtime_error("streamlines must have 'vertex' array");
    if (vertexData->type != OSP_FLOAT4)
      throw std::runtime_error("streamlines 'vertex' must be type OSP_FLOAT4");
    auto vertex = (vec4f*)vertexData->data;
    numVertices = vertexData->numItems;

    indexData  = getParamData("index",nullptr);
    if (!indexData)
      throw std::runtime_error("streamlines must have 'index' array");
    if (indexData->type != OSP_INT)
      throw std::runtime_error("streamlines 'index' array must be type OSP_INT");
    index = (uint32*)indexData->data;

    postStatusMsg(2) << "#osp: creating streamlines geometry, "
                     << "#verts=" << numVertices << ", "
                     << "#segments=" << numSegments;

    bounds = empty;
    for (uint32_t i = 0; i < indexData->numItems; i++) {
      const uint32_t idx = index[i];
      for (uint32_t v = idx; v < idx + 4; v++) {
        float radius = vertex[v][3];
        vec3f vtx(vertex[v].x, vertex[v].y, vertex[v].z);
        bounds.extend(vtx - radius);
        bounds.extend(vtx + radius);
      }
    }

    ispc::Curves_set(getIE(),
                     model->getIE(),
                     (const ispc::vec4f*)vertexData->data,
                     vertexData->numItems,
                     (const uint32_t*)indexData->data,
                     indexData->numItems);
  }

  OSP_REGISTER_GEOMETRY(Curves,curves);

} // ::ospray
