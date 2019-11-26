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
#include <map>
#include "Curves_ispc.h"
#include "ospcommon/utility/DataView.h"

namespace ospray {
  static std::map<std::pair<OSPCurveType, OSPCurveBasis>, RTCGeometryType>
      curveMap = {
          {{OSP_ROUND,  OSP_LINEAR},  RTC_GEOMETRY_TYPE_USER},
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
          {{OSP_RIBBON, OSP_HERMITE}, RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_HERMITE_CURVE},
          
          {{OSP_ROUND,  OSP_CATMULL_ROM}, RTC_GEOMETRY_TYPE_ROUND_CATMULL_ROM_CURVE},
          {{OSP_FLAT,   OSP_CATMULL_ROM}, RTC_GEOMETRY_TYPE_FLAT_CATMULL_ROM_CURVE},
          {{OSP_RIBBON, OSP_CATMULL_ROM}, RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_CATMULL_ROM_CURVE}};

  std::string Curves::toString() const
  {
    return "ospray::Curves";
  }

  void Curves::commit()
  {
    indexData = getParamDataT<uint32_t>("index", true);
    normalData = nullptr;
    tangentData = nullptr;
    vertexData = getParamDataT<vec3f>("vertex.position");
    texcoordData = getParamDataT<vec2f>("vertex.texcoord");

    if (vertexData) { // round, linear curves with constant radius
      radius = getParam<float>("radius", 0.01f);
      curveType = (OSPCurveType)getParam<int>("type", OSP_ROUND);
      curveBasis = (OSPCurveBasis)getParam<int>("basis", OSP_LINEAR);
      if (curveMap[std::make_pair(curveType, curveBasis)]
          != RTC_GEOMETRY_TYPE_USER)
        throw std::runtime_error(
            "constant-radius curves need to be of type OSP_ROUND and have basis OSP_LINEAR");
    } else { // embree curves
      vertexData = getParamDataT<vec4f>("vertex.position_radius", true);
      radius = 0.0f;

      curveType = (OSPCurveType)getParam<int>("type", OSP_UNKNOWN_CURVE_TYPE);
      if (curveType == OSP_UNKNOWN_CURVE_TYPE)
        throw std::runtime_error("curves geometry has invalid 'type'");

      curveBasis =
          (OSPCurveBasis)getParam<int>("basis", OSP_UNKNOWN_CURVE_BASIS);
      if (curveBasis == OSP_UNKNOWN_CURVE_BASIS)
        throw std::runtime_error("curves geometry has invalid 'basis'");

      if (curveBasis == OSP_LINEAR && curveType != OSP_FLAT)
        throw std::runtime_error(
            "curves geometry with linear basis must be of flat type or have constant radius");

      if (curveType == OSP_RIBBON)
        normalData = getParamDataT<vec3f>("vertex.normal", true);

      if (curveBasis == OSP_HERMITE)
        tangentData = getParamDataT<vec4f>("vertex.tangent", true);
    }

    colorData = getParamDataT<vec4f>("vertex.color");

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

    LiveGeometry retval;
    retval.embreeGeometry = embreeGeo;
    retval.ispcEquivalent = ispc::Curves_create(this);

    if (embreeCurveType == RTC_GEOMETRY_TYPE_USER) {
      ispc::Curves_setUserGeometry(retval.ispcEquivalent,
          retval.embreeGeometry,
          radius,
          ispc(indexData),
          ispc(vertexData),
          ispc(colorData),
          ispc(texcoordData));
    } else {
      Ref<const DataT<vec4f>> vertex4f(&vertexData->as<vec4f>());
      setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_VERTEX, vertex4f);
      setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_INDEX, indexData);
      setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_NORMAL, normalData);
      setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_TANGENT, tangentData);
      if (colorData) {
        rtcSetGeometryVertexAttributeCount(embreeGeo, 1);
        setEmbreeGeometryBuffer(
            embreeGeo, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, colorData);
      }
      if (texcoordData) {
      rtcSetGeometryVertexAttributeCount(embreeGeo, 2);
      setEmbreeGeometryBuffer(
          embreeGeo, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, texcoordData, 1);
      }
      ispc::Curves_set(retval.ispcEquivalent,
          retval.embreeGeometry,
          colorData,
          texcoordData,
          indexData->size());
    }

    rtcCommitGeometry(embreeGeo);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(Curves, curves);

}  // namespace ospray
