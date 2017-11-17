// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "StreamLines.h"
#include "common/Data.h"
#include "common/Model.h"
#include "ospcommon/utility/DataView.h"
// ispc-generated files
#include "StreamLines_ispc.h"

namespace ospray {

  StreamLines::StreamLines()
  {
    this->ispcEquivalent = ispc::StreamLines_create(this);
  }

  std::string StreamLines::toString() const
  {
    return "ospray::StreamLines";
  }

  void StreamLines::finalize(Model *model)
  {
    Geometry::finalize(model);

    const float globalRadius = getParam1f("radius",0.01f);
    utility::DataView<const float> radius(&globalRadius, 0);
    bool useCurve = getParam1i("smooth", 0);

    vertexData = getParamData("vertex",nullptr);
    if (!vertexData)
      throw std::runtime_error("streamlines must have 'vertex' array");
    if (vertexData->type != OSP_FLOAT4 && vertexData->type != OSP_FLOAT3A)
      throw std::runtime_error("'vertex' must have data type OSP_FLOAT4 or OSP_FLOAT3A");
    vertex = (vec3fa*)vertexData->data;
    numVertices = vertexData->numItems;
    if (vertexData->type == OSP_FLOAT4) {
      radius.reset((const float*)vertex + 3, 4);
      useCurve = true;
    }

    indexData  = getParamData("index",nullptr);
    if (!indexData)
      throw std::runtime_error("streamlinee must have 'index' array");
    if (indexData->type != OSP_INT)
      throw std::runtime_error("'index' must have data type OSP_INT");
    index = (uint32*)indexData->data;
    numSegments = indexData->numItems;

    colorData = getParamData("vertex.color",getParamData("color"));
    if (colorData && colorData->type != OSP_FLOAT4)
      throw std::runtime_error("'vertex.color' must have data type OSP_FLOAT4");
    color = colorData ? (vec4f*)colorData->data : nullptr;

    radiusData = getParamData("vertex.radius");
    if (radiusData && radiusData->type == OSP_FLOAT) {
      radius.reset((const float*)radiusData->data);
      useCurve = true;
    }

    postStatusMsg(2) << "#osp: creating streamlines geometry, "
                     << "#verts=" << numVertices << ", "
                     << "#segments=" << numSegments << ", "
                     << "as curve: " << useCurve;

    bounds = empty;
    for (uint32_t i = 0; i < numVertices; i++) {
      const float radiusI = radius[i];
      bounds.extend(vertex[i] - radiusI);
      bounds.extend(vertex[i] + radiusI);
    }

    if (useCurve) {
      numVertices += numSegments*2;
      vec4f* newvertex = new vec4f[numVertices];
      uint32_t* newindex = new uint32_t[numSegments];
      uint32_t vidx = 0;
      for (uint32_t i = 0; i < numSegments; i++) {
        const uint32 idx = index[i];
        const vec3f start = vertex[idx];
        const vec3f end = vertex[idx+1];
        const float startRadius = radius[idx];
        const float endRadius = radius[idx+1];
        if (i+1 < numSegments && index[i+1] == idx+1) { // inter-link
          const vec3f next = vertex[idx+2];
          const vec3f delta = 0.25f*(next - start);
          const float a = length(start - end);
          const float b = length(next - end);
          const float r = a/(a+b);
          newvertex[vidx] = vec4f(start, startRadius);
          newvertex[vidx+2] = vec4f(end - r*delta, startRadius);
          newvertex[vidx+3] = vec4f(end, endRadius);
          newvertex[vidx+4] = vec4f(end + (1.f-r)*delta, endRadius);
          newindex[i] = vidx;
          vidx += 3;
        } else { // end
          newvertex[vidx] = vec4f(start, startRadius);
          newvertex[vidx+1] = vec4f(start + (end-start)*(1.f/3), startRadius);
          newvertex[vidx+2] = vec4f(start + (end-start)*(2.f/3), endRadius);
          newvertex[vidx+3] = vec4f(end, endRadius);
          vidx += 4;
        }
      }
      ispc::StreamLines_setCurve(getIE(),model->getIE(),
                                 (const ispc::vec3fa*)newvertex, numVertices,
                                 (const uint32_t*)newindex, numSegments,
                                 (const ispc::vec4f*)color);
    } else
      ispc::StreamLines_set(getIE(),model->getIE(), globalRadius,
                            (const ispc::vec3fa*)vertex,
                            numVertices, (const uint32_t*)index, numSegments,
                            (const ispc::vec4f*)color);
  }

  OSP_REGISTER_GEOMETRY(StreamLines,streamlines);

} // ::ospray
