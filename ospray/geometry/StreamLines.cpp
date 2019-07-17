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
#include "StreamLines.h"
#include "common/Data.h"
#include "common/World.h"
#include "ospcommon/utility/DataView.h"
// ispc-generated files
#include "StreamLines_ispc.h"

namespace ospray {

  std::string StreamLines::toString() const
  {
    return "ospray::StreamLines";
  }

  void StreamLines::commit()
  {
    useCurve     = getParam1b("smooth", false);
    globalRadius = getParam1f("radius", 0.01f);
    utility::DataView<const float> radius(&globalRadius, 0);

    indexData = getParamData("index", nullptr);

    if (!indexData)
      throw std::runtime_error("streamlines must have 'index' array");
    if (indexData->type != OSP_INT)
      throw std::runtime_error(
          "streamlines 'index' array must be type OSP_INT");

    index       = (uint32 *)indexData->data;
    numSegments = indexData->numItems;

    vertexData = getParamData("vertex.position", nullptr);

    if (!vertexData)
      throw std::runtime_error("streamlines must have 'vertex.position' array");
    if (vertexData->type != OSP_VEC4F && vertexData->type != OSP_VEC3FA)
      throw std::runtime_error(
          "streamlines 'vertex.position' must be type OSP_VEC4F or OSP_VEC3FA");

    vertex = (vec3fa *)vertexData->data;

    if (vertexData->type == OSP_VEC4F) {
      radius.reset((const float *)vertex + 3, sizeof(vec4f));
      useCurve = true;
    }

    colorData = getParamData("vertex.color");

    if (colorData && colorData->type != OSP_VEC4F)
      throw std::runtime_error("'vertex.color' must have data type OSP_VEC4F");

    radiusData = getParamData("vertex.radius");

    if (radiusData && radiusData->type == OSP_FLOAT) {
      radius.reset((const float *)radiusData->data);
      useCurve = true;
    }

    if (useCurve) {
      vertexCurve.clear();
      indexCurve.resize(numSegments);
      bool middleSegment = false;
      vec3f tangent;
      for (uint32_t i = 0; i < numSegments; i++) {
        const uint32 idx          = index[i];
        const vec3f start         = vertex[idx];
        const vec3f end           = vertex[idx + 1];
        const float lengthSegment = length(start - end);
        const float startRadius   = radius[idx];
        const float endRadius     = radius[idx + 1];

        indexCurve[i] = vertexCurve.size();
        if (middleSegment) {
          vertexCurve.push_back(vec4f(start, startRadius));
          vertexCurve.push_back(
              vec4f(start + tangent, lerp(1.f / 3, startRadius, endRadius)));
        } else {  // start new curve
          const vec3f cap = lerp(1.f + startRadius / lengthSegment, end, start);
          vertexCurve.push_back(vec4f(cap, 0.f));
          vertexCurve.push_back(vec4f(start, startRadius));
        }

        middleSegment = i + 1 < numSegments && index[i + 1] == idx + 1;
        if (middleSegment) {
          const vec3f next  = vertex[idx + 2];
          const vec3f delta = (1.f / 3) * (next - start);
          const float b     = length(next - end);
          const float r     = lengthSegment / (lengthSegment + b);
          vertexCurve.push_back(
              vec4f(end - r * delta, lerp(2.f / 3, startRadius, endRadius)));
          tangent = (1.f - r) * delta;
        } else {  // end curve
          vertexCurve.push_back(vec4f(end, endRadius));
          const vec3f cap = lerp(1.f + endRadius / lengthSegment, start, end);
          vertexCurve.push_back(vec4f(cap, 0.f));
        }
      }
    }

    numVertices = useCurve ? vertexCurve.size() : vertexData->numItems;

    postStatusMsg(2) << "#osp: creating streamlines geometry, "
                     << "#verts=" << numVertices << ", "
                     << "#segments=" << numSegments << ", "
                     << "as curve: " << useCurve;
  }

  size_t StreamLines::numPrimitives() const
  {
    return numSegments;
  }

  LiveGeometry StreamLines::createEmbreeGeometry()
  {
    LiveGeometry retval;

    retval.ispcEquivalent = ispc::StreamLines_create(this);
    retval.embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(),
                       useCurve ? RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE
                                : RTC_GEOMETRY_TYPE_USER);

    if (useCurve) {
      rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                                 RTC_BUFFER_TYPE_VERTEX,
                                 0,
                                 RTC_FORMAT_FLOAT4,
                                 vertexCurve.data(),
                                 0,
                                 sizeof(vec4f),
                                 numVertices);

      rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                                 RTC_BUFFER_TYPE_INDEX,
                                 0,
                                 RTC_FORMAT_UINT,
                                 indexCurve.data(),
                                 0,
                                 sizeof(int),
                                 numSegments);

      ispc::StreamLines_setCurve(
          retval.ispcEquivalent,
          vertexCurve.size(),
          numSegments,
          index,
          colorData ? (ispc::vec4f *)colorData->data : nullptr);
    } else {
      ispc::StreamLines_set(
          retval.ispcEquivalent,
          retval.embreeGeometry,
          globalRadius,
          (const ispc::vec3fa *)vertex,
          numVertices,
          index,
          numSegments,
          colorData ? (ispc::vec4f *)colorData->data : nullptr);
    }

    rtcCommitGeometry(retval.embreeGeometry);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(StreamLines, streamlines);

}  // namespace ospray
