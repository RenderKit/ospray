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

  enum curveType { ROUND, FLAT, RIBBON };
  enum curveBasis { LINEAR, BEZIER, BSPLINE, HERMITE };

  static RTCGeometryType curveMap[4][3] =
    {
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

      {
        RTC_GEOMETRY_TYPE_ROUND_HERMITE_CURVE,
        RTC_GEOMETRY_TYPE_FLAT_HERMITE_CURVE,
        RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_HERMITE_CURVE
      }
    };

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
      throw std::runtime_error("curves must have 'vertex' array");
    if (vertexData->type != OSP_FLOAT4)
      throw std::runtime_error("curves 'vertex' must be type OSP_FLOAT4");
    auto vertex = (vec4f*)vertexData->data;
    auto numVertices = vertexData->numItems;

    indexData  = getParamData("index",nullptr);
    if (!indexData)
      throw std::runtime_error("curves must have 'index' array");
    if (indexData->type != OSP_INT)
      throw std::runtime_error("curves 'index' array must be type OSP_INT");
    auto index = (uint32*)indexData->data;
    auto numSegments = indexData->numItems;

    normalData = getParamData("vertex.normal", nullptr);
    tangentData = getParamData("vertex.tangent", nullptr);

    auto basis = curveBasisForString(getParamString("curveBasis", "unspecified"));
    auto type = curveTypeForString(getParamString("curveType", "unspecified"));

    if (type == RIBBON && !normalData)
      throw std::runtime_error("ribbon curve must have 'normal' array");
    if (basis == LINEAR && type != FLAT)
      throw std::runtime_error("linear curve with non-flat type");
    if (basis == HERMITE && !tangentData)
      throw std::runtime_error("hermite curve must have 'tangent' array");

    if (normalData && normalData->type != OSP_FLOAT3)
      throw std::runtime_error("curves 'normal' array must be type OSP_FLOAT3");
    if (tangentData && tangentData->type != OSP_FLOAT3)
      throw std::runtime_error("curves 'tangent' array must be type OSP_FLOAT3");

    postStatusMsg(2) << "#osp: creating curves geometry, "
                     << "#verts=" << numVertices << ", "
                     << "#segments=" << numSegments;

    uint32_t numVerts = 4;
    if (basis == LINEAR || basis == HERMITE)
      numVerts = 2;

    bounds = empty;
    for (uint32_t i = 0; i < indexData->numItems; i++) {
      const uint32_t idx = index[i];
      for (uint32_t v = idx; v < idx + numVerts; v++) {
        float radius = vertex[v].w;
        vec3f vtx(vertex[v].x, vertex[v].y, vertex[v].z);
        bounds.extend(vtx - radius);
        bounds.extend(vtx + radius);
      }
    }

    ispc::Curves_set(getIE(),
                     model->getIE(),
                     (ispc::RTCGeometryType)curveMap[basis][type],
                     (const ispc::vec4f*)vertexData->data,
                     vertexData->numItems,
                     (const uint32_t*)indexData->data,
                     indexData->numItems,
                     normalData ? (const ispc::vec3f*)normalData->data : nullptr,
                     normalData ? normalData->numItems : 0,
                     tangentData ? (const ispc::vec3f*)tangentData->data : nullptr,
                     tangentData ? tangentData->numItems : 0);
  }

  OSP_REGISTER_GEOMETRY(Curves,curves);

} // ::ospray
