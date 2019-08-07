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
#include "Curves.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "Curves_ispc.h"

#include <map>

namespace ospray {
  static std::map<std::pair<OSPCurveType, OSPCurveBasis>, RTCGeometryType>
      curveMap = {
          {{OSP_ROUND,  OSP_LINEAR},  (RTCGeometryType)-1},
          {{OSP_FLAT,   OSP_LINEAR},  RTC_GEOMETRY_TYPE_FLAT_LINEAR_CURVE},
          {{OSP_RIBBON, OSP_LINEAR},  (RTCGeometryType)-1},

          {{OSP_ROUND,  OSP_BEZIER},  RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE},
          {{OSP_FLAT,   OSP_BEZIER},  RTC_GEOMETRY_TYPE_FLAT_BEZIER_CURVE},
          {{OSP_RIBBON, OSP_BEZIER},  RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BEZIER_CURVE},

          {{OSP_ROUND,  OSP_BSPLINE}, RTC_GEOMETRY_TYPE_ROUND_BSPLINE_CURVE},
          {{OSP_FLAT,   OSP_BSPLINE}, RTC_GEOMETRY_TYPE_FLAT_BSPLINE_CURVE},
          {{OSP_RIBBON, OSP_BSPLINE}, RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BSPLINE_CURVE},

          {{OSP_ROUND,  OSP_HERMITE}, RTC_GEOMETRY_TYPE_ROUND_HERMITE_CURVE},
          {{OSP_FLAT,   OSP_HERMITE}, RTC_GEOMETRY_TYPE_FLAT_HERMITE_CURVE},
          {{OSP_RIBBON, OSP_HERMITE}, RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_HERMITE_CURVE}};

  std::string Curves::toString() const
  {
    return "ospray::Curves";
  }

  void Curves::commit()
  {
    vertexData = getParamData("vertex.position", nullptr);
    if (!vertexData) {
      throw std::runtime_error(
          "curves geometry must have 'vertex.position' array");
    }

    if (vertexData->type != OSP_VEC4F) {
      throw std::runtime_error(
          "curves geometry 'vertex.position' array must have element type "
          "OSP_VEC4F");
    }

    const auto numVertices = vertexData->numItems;

    normalData  = getParamData("vertex.normal", nullptr);
    tangentData = getParamData("vertex.tangent", nullptr);

    indexData = getParamData("index", nullptr);
    if (!indexData) {
      throw std::runtime_error("curves must have 'index' array");
    }

    if (indexData->type != OSP_INT) {
      throw std::runtime_error(
          "curves geometry 'index' array must have element type OSP_INT");
    }

    const auto numSegments = indexData->numItems;

    curveType = (OSPCurveType)getParam<int>("type", OSP_UNKNOWN_CURVE_TYPE);
    if (curveType == OSP_UNKNOWN_CURVE_TYPE)
      throw std::runtime_error("curves geometry has invalid 'type'");

    curveBasis = (OSPCurveBasis)getParam<int>("basis", OSP_UNKNOWN_CURVE_BASIS);
    if (curveBasis == OSP_UNKNOWN_CURVE_BASIS)
      throw std::runtime_error("curves geometry has invalid 'basis'");

    if (curveType == OSP_RIBBON && !normalData) {
      throw std::runtime_error(
          "curves geometry with ribbon type must have 'normal' array");
    }

    if (curveBasis == OSP_LINEAR && curveType != OSP_FLAT) {
      throw std::runtime_error(
          "curves geometry with linear basis must be flat type");
    }

    if (curveBasis == OSP_HERMITE && !tangentData) {
      throw std::runtime_error(
          "curves geometry with hermite basis must have 'tangent' array");
    }

    if (normalData && normalData->type != OSP_VEC3F) {
      throw std::runtime_error(
          "curves geometry 'normal' array must have element type OSP_VEC3F");
    }

    if (tangentData && tangentData->type != OSP_VEC3F) {
      throw std::runtime_error(
          "curves geometry 'tangent' array must have element type OSP_VEC3F");
    }

    postStatusMsg(2) << "#osp: creating curves geometry, "
                     << "#verts=" << numVertices << ", "
                     << "#segments=" << numSegments;

    embreeCurveType = curveMap[std::make_pair(curveType, curveBasis)];
  }

  size_t Curves::numPrimitives() const
  {
    return indexData->numItems;
  }

  LiveGeometry Curves::createEmbreeGeometry()
  {
    LiveGeometry retval;

    retval.embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), embreeCurveType);
    retval.ispcEquivalent = ispc::Curves_create(this);

    rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                               RTC_BUFFER_TYPE_VERTEX,
                               0,
                               RTC_FORMAT_FLOAT4,
                               vertexData->data,
                               0,
                               sizeof(vec4f),
                               vertexData->numItems);

    rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                               RTC_BUFFER_TYPE_INDEX,
                               0,
                               RTC_FORMAT_UINT,
                               indexData->data,
                               0,
                               sizeof(int),
                               indexData->numItems);

    if (normalData) {
      rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                                 RTC_BUFFER_TYPE_NORMAL,
                                 0,
                                 RTC_FORMAT_FLOAT3,
                                 normalData->data,
                                 0,
                                 sizeof(vec3f),
                                 normalData->numItems);
    }

    if (tangentData) {
      rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                                 RTC_BUFFER_TYPE_TANGENT,
                                 0,
                                 RTC_FORMAT_FLOAT3,
                                 tangentData->data,
                                 0,
                                 sizeof(vec3f),
                                 tangentData->numItems);
    }

    rtcCommitGeometry(retval.embreeGeometry);

    ispc::Curves_set(retval.ispcEquivalent,
                     (ispc::RTCGeometryType)embreeCurveType,
                     indexData->numItems);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(Curves, curves);

}  // namespace ospray
