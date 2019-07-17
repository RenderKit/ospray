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

namespace ospray {

  enum curveType
  {
    ROUND,
    FLAT,
    RIBBON
  };

  enum curveBasis
  {
    LINEAR,
    BEZIER,
    BSPLINE,
    HERMITE
  };

  static RTCGeometryType curveMap[4][3] = {
      {
          (RTCGeometryType)-1,
          RTC_GEOMETRY_TYPE_FLAT_LINEAR_CURVE,
          (RTCGeometryType)-1,
      },

      {
          RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE,
          RTC_GEOMETRY_TYPE_FLAT_BEZIER_CURVE,
          RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BEZIER_CURVE,
      },

      {
          RTC_GEOMETRY_TYPE_ROUND_BSPLINE_CURVE,
          RTC_GEOMETRY_TYPE_FLAT_BSPLINE_CURVE,
          RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BSPLINE_CURVE,
      },

      {RTC_GEOMETRY_TYPE_ROUND_HERMITE_CURVE,
       RTC_GEOMETRY_TYPE_FLAT_HERMITE_CURVE,
       RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_HERMITE_CURVE}};

  static curveType curveTypeForString(const std::string &s)
  {
    if (s == "flat")
      return FLAT;
    if (s == "ribbon")
      return RIBBON;
    if (s == "round")
      return ROUND;
    throw std::runtime_error("curve with unknown curveType");
  }

  static curveBasis curveBasisForString(const std::string &s)
  {
    if (s == "bezier")
      return BEZIER;
    if (s == "bspline")
      return BSPLINE;
    if (s == "linear")
      return LINEAR;
    if (s == "hermite")
      return HERMITE;
    throw std::runtime_error("curve with unknown curveBasis");
  }

  std::string Curves::toString() const
  {
    return "ospray::Curves";
  }

  void Curves::commit()
  {
    vertexData = getParamData("vertex.position", nullptr);
    if (!vertexData)
      throw std::runtime_error("curves must have 'vertex' array");
    if (vertexData->type != OSP_VEC4F)
      throw std::runtime_error("curves 'vertex' must be type OSP_FLOAT4");
    const auto numVertices = vertexData->numItems;

    normalData  = getParamData("vertex.normal", nullptr);
    tangentData = getParamData("vertex.tangent", nullptr);

    indexData = getParamData("index", nullptr);
    if (!indexData)
      throw std::runtime_error("curves must have 'index' array");
    if (indexData->type != OSP_INT)
      throw std::runtime_error("curves 'index' array must be type OSP_INT");
    const auto numSegments = indexData->numItems;

    curveBasis = getParamString("basis", "unspecified");
    curveType  = getParamString("type", "unspecified");

    const auto type  = curveTypeForString(curveType);
    const auto basis = curveBasisForString(curveBasis);

    if (type == RIBBON && !normalData)
      throw std::runtime_error("ribbon curve must have 'normal' array");
    if (basis == LINEAR && type != FLAT)
      throw std::runtime_error("linear curve with non-flat type");
    if (basis == HERMITE && !tangentData)
      throw std::runtime_error("hermite curve must have 'tangent' array");

    if (normalData && normalData->type != OSP_VEC3F)
      throw std::runtime_error("curves 'normal' array must be type OSP_VEC3F");
    if (tangentData && tangentData->type != OSP_VEC3F)
      throw std::runtime_error("curves 'tangent' array must be type OSP_VEC3F");

    postStatusMsg(2) << "#osp: creating curves geometry, "
                     << "#verts=" << numVertices << ", "
                     << "#segments=" << numSegments;

    embreeCurveType = curveMap[basis][type];
  }

  size_t Curves::numPrimitives() const
  {
    const bool haveIndices = indexData;

    if (!haveIndices)
      return 0;

    const auto basis = curveBasisForString(curveBasis);

    uint32_t numVerts = 4;
    if (basis == LINEAR || basis == HERMITE)
      numVerts = 2;

    return indexData->numItems / numVerts;
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
