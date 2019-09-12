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
    vertexData = getParamDataT<vec4f>("vertex.position", true);
    indexData = getParamDataT<uint32_t>("index", true);

    curveType = (OSPCurveType)getParam<int>("type", OSP_UNKNOWN_CURVE_TYPE);
    if (curveType == OSP_UNKNOWN_CURVE_TYPE)
      throw std::runtime_error("curves geometry has invalid 'type'");

    curveBasis = (OSPCurveBasis)getParam<int>("basis", OSP_UNKNOWN_CURVE_BASIS);
    if (curveBasis == OSP_UNKNOWN_CURVE_BASIS)
      throw std::runtime_error("curves geometry has invalid 'basis'");

    normalData = curveType == OSP_RIBBON
        ? getParamDataT<vec3f>("vertex.normal", true)
        : nullptr;

    tangentData = curveBasis == OSP_HERMITE
        ? getParamDataT<vec3f>("vertex.tangent", true)
        : nullptr;

    if (curveBasis == OSP_LINEAR && curveType != OSP_FLAT)
      throw std::runtime_error(
          "curves geometry with linear basis must be of flat type");

    embreeCurveType = curveMap[std::make_pair(curveType, curveBasis)];

    postCreationInfo(vertexData->size());
  }

  size_t Curves::numPrimitives() const
  {
    return indexData->size();
  }

  LiveGeometry Curves::createEmbreeGeometry()
  {
    auto embreeGeo = rtcNewGeometry(ispc_embreeDevice(), embreeCurveType);

    setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_VERTEX, vertexData);
    setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_INDEX, indexData);
    setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_NORMAL, normalData);
    setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_TANGENT, tangentData);

    rtcCommitGeometry(embreeGeo);

    LiveGeometry retval;
    retval.embreeGeometry = embreeGeo;
    retval.ispcEquivalent = ispc::Curves_create(this);

    ispc::Curves_set(retval.ispcEquivalent,
        (ispc::RTCGeometryType)embreeCurveType,
        indexData->size());

    return retval;
  }

  OSP_REGISTER_GEOMETRY(Curves, curves);

}  // namespace ospray
