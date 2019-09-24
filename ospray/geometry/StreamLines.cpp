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
    vertexData = getParamDataT<vec3f>("vertex.position", true);
    indexData = getParamDataT<uint32_t>("index", true);

    useCurve = getParam<bool>("smooth", false);
    radius = getParam<float>("radius", 0.01f);
    utility::DataView<const float> radiusView(&radius, 0);

    colorData = getParamDataT<vec4f>("vertex.color");
    radiusData = getParamDataT<float>("vertex.radius");

    if (radiusData) {
      radiusView.reset(radiusData->data(), radiusData->stride());
      useCurve = true;
    }

    if (useCurve) {
      const auto numSegments = indexData->size();
      vertexCurve.clear();
      indexCurve.resize(numSegments);
      bool middleSegment = false;
      vec3f tangent;
      const auto &vertex = *vertexData;
      const auto &index = *indexData;
      for (uint32_t i = 0; i < numSegments; i++) {
        const uint32 idx          = index[i];
        const vec3f start         = vertex[idx];
        const vec3f end           = vertex[idx + 1];
        const float lengthSegment = length(start - end);
        const float startRadius = radiusView[idx];
        const float endRadius = radiusView[idx + 1];

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
    } else {
      vertexCurve.clear();
      indexCurve.clear();
    }

    postCreationInfo(useCurve ? vertexCurve.size() : vertexData->size());
  }

  size_t StreamLines::numPrimitives() const
  {
    return indexData ? indexData->size() : 0;
  }

  LiveGeometry StreamLines::createEmbreeGeometry()
  {
    auto embreeGeo = rtcNewGeometry(ispc_embreeDevice(),
        useCurve ? RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE
                 : RTC_GEOMETRY_TYPE_USER);

    LiveGeometry retval;
    retval.embreeGeometry = embreeGeo;
    retval.ispcEquivalent = ispc::StreamLines_create(this);

    if (useCurve) {
      setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_INDEX, indexCurve);
      setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_VERTEX, vertexCurve);

      ispc::StreamLines_setCurve(
          retval.ispcEquivalent, ispc(indexData), ispc(colorData));
    } else {
      ispc::StreamLines_set(retval.ispcEquivalent,
          retval.embreeGeometry,
          radius,
          ispc(indexData),
          ispc(vertexData),
          ispc(colorData));
    }

    rtcCommitGeometry(embreeGeo);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(StreamLines, streamlines);

}  // namespace ospray
