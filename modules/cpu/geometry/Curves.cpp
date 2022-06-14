// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Curves.h"
#include "common/DGEnum.h"

// ispc-generated files
#include "geometry/Curves_ispc.h"
// std
#include <map>

namespace ospray {

static std::map<std::pair<OSPCurveType, OSPCurveBasis>, RTCGeometryType>
    curveMap = {{{OSP_ROUND, OSP_LINEAR}, RTC_GEOMETRY_TYPE_ROUND_LINEAR_CURVE},
        {{OSP_FLAT, OSP_LINEAR}, RTC_GEOMETRY_TYPE_FLAT_LINEAR_CURVE},
        {{OSP_RIBBON, OSP_LINEAR}, (RTCGeometryType)-1},
        {{OSP_DISJOINT, OSP_LINEAR}, RTC_GEOMETRY_TYPE_CONE_LINEAR_CURVE},

        {{OSP_ROUND, OSP_BEZIER}, RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE},
        {{OSP_FLAT, OSP_BEZIER}, RTC_GEOMETRY_TYPE_FLAT_BEZIER_CURVE},
        {{OSP_RIBBON, OSP_BEZIER},
            RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BEZIER_CURVE},
        {{OSP_DISJOINT, OSP_BEZIER}, (RTCGeometryType)-1},

        {{OSP_ROUND, OSP_BSPLINE}, RTC_GEOMETRY_TYPE_ROUND_BSPLINE_CURVE},
        {{OSP_FLAT, OSP_BSPLINE}, RTC_GEOMETRY_TYPE_FLAT_BSPLINE_CURVE},
        {{OSP_RIBBON, OSP_BSPLINE},
            RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BSPLINE_CURVE},
        {{OSP_DISJOINT, OSP_BSPLINE}, (RTCGeometryType)-1},

        {{OSP_ROUND, OSP_HERMITE}, RTC_GEOMETRY_TYPE_ROUND_HERMITE_CURVE},
        {{OSP_FLAT, OSP_HERMITE}, RTC_GEOMETRY_TYPE_FLAT_HERMITE_CURVE},
        {{OSP_RIBBON, OSP_HERMITE},
            RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_HERMITE_CURVE},
        {{OSP_DISJOINT, OSP_HERMITE}, (RTCGeometryType)-1},

        {{OSP_ROUND, OSP_CATMULL_ROM},
            RTC_GEOMETRY_TYPE_ROUND_CATMULL_ROM_CURVE},
        {{OSP_FLAT, OSP_CATMULL_ROM}, RTC_GEOMETRY_TYPE_FLAT_CATMULL_ROM_CURVE},
        {{OSP_RIBBON, OSP_CATMULL_ROM},
            RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_CATMULL_ROM_CURVE},
        {{OSP_DISJOINT, OSP_CATMULL_ROM}, (RTCGeometryType)-1}};

// Curves definitions ///////////////////////////////////////////////////////

Curves::Curves()
{
  getSh()->super.postIntersect = ispc::Curves_postIntersect_addr();
  // TODO implement area sampling of OldCurves for geometry lights
}

std::string Curves::toString() const
{
  return "ospray::Curves";
}

void Curves::commit()
{
  indexData = getParamDataT<uint32_t>("index", true);
  normalData = nullptr;
  tangentData = nullptr;
  texcoordData = getParamDataT<vec2f>("vertex.texcoord");

  Ref<const DataT<vec3f>> vertexNoRadiusData =
      getParamDataT<vec3f>("vertex.position");
  if (vertexNoRadiusData) { // round, linear curves with constant radius
    float radius = getParam<float>("radius", 0.01f);
    curveType = (OSPCurveType)getParam<uint8_t>(
        "type", getParam<int32_t>("type", OSP_ROUND));
    curveBasis = (OSPCurveBasis)getParam<uint8_t>(
        "basis", getParam<int32_t>("basis", OSP_LINEAR));
    if (curveMap[std::make_pair(curveType, curveBasis)]
        != RTC_GEOMETRY_TYPE_ROUND_LINEAR_CURVE)
      throw std::runtime_error(
          "constant-radius curves need to be of type OSP_ROUND and have "
          "basis OSP_LINEAR");

    // To maintain OSPRay 2.x compatibility and keep support for
    // the global 'radius' parameter, a vec4f vertex buffer copy
    // has to be created. It specifies radius on per-vertex basis and
    // is required by Embree. TODO: Refactor for OSPRay 3.x
    DataT<vec4f> *dataT =
        (DataT<vec4f> *)new Data(OSP_VEC4F, vertexNoRadiusData->numItems);
    for (size_t i = 0; i < dataT->size(); i++) {
      const vec3f &v = (*vertexNoRadiusData)[i];
      (*dataT)[i] = vec4f(v.x, v.y, v.z, radius);
    }
    vertexData = dataT;
    dataT->refDec();
  } else { // embree curves
    vertexData = getParamDataT<vec4f>("vertex.position_radius", true);

    curveType = (OSPCurveType)getParam<uint8_t>(
        "type", getParam<int32_t>("type", OSP_UNKNOWN_CURVE_TYPE));
    if (curveType == OSP_UNKNOWN_CURVE_TYPE)
      throw std::runtime_error("curves geometry has invalid 'type'");

    curveBasis = (OSPCurveBasis)getParam<uint8_t>(
        "basis", getParam<int32_t>("basis", OSP_UNKNOWN_CURVE_BASIS));
    if (curveBasis == OSP_UNKNOWN_CURVE_BASIS)
      throw std::runtime_error("curves geometry has invalid 'basis'");

    if (curveType == OSP_RIBBON)
      normalData = getParamDataT<vec3f>("vertex.normal", true);

    if (curveBasis == OSP_HERMITE)
      tangentData = getParamDataT<vec4f>("vertex.tangent", true);
  }

  colorData = getParamDataT<vec4f>("vertex.color");

  embreeCurveType = curveMap[std::make_pair(curveType, curveBasis)];
  if (embreeCurveType == (RTCGeometryType)-1)
    throw std::runtime_error("unsupported combination of 'type' and 'basis'");

  createEmbreeGeometry();

  getSh()->geom = embreeGeometry;
  getSh()->flagMask = -1;
  if (!colorData)
    getSh()->flagMask &= ispc::int64(~DG_COLOR);
  if (!texcoordData)
    getSh()->flagMask &= ispc::int64(~DG_TEXCOORD);
  getSh()->super.numPrimitives = numPrimitives();

  postCreationInfo(vertexData->size());
}

size_t Curves::numPrimitives() const
{
  return indexData->size();
}

void Curves::createEmbreeGeometry()
{
  Geometry::createEmbreeGeometry(embreeCurveType);

  Ref<const DataT<vec4f>> vertex4f(&vertexData->as<vec4f>());
  setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX, vertex4f);
  setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_INDEX, indexData);
  setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_NORMAL, normalData);
  setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_TANGENT, tangentData);
  if (colorData) {
    rtcSetGeometryVertexAttributeCount(embreeGeometry, 1);
    setEmbreeGeometryBuffer(
        embreeGeometry, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, colorData);
  }
  if (texcoordData) {
    rtcSetGeometryVertexAttributeCount(embreeGeometry, 2);
    setEmbreeGeometryBuffer(
        embreeGeometry, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, texcoordData, 1);
  }

  rtcCommitGeometry(embreeGeometry);
}

} // namespace ospray
