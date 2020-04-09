// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Curves.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "Curves_ispc.h"
// ospcommon
#include "ospcommon/utility/DataView.h"
// std
#include <map>

namespace ospray {

static std::map<std::pair<OSPCurveType, OSPCurveBasis>, RTCGeometryType>
    curveMap = {{{OSP_ROUND, OSP_LINEAR}, RTC_GEOMETRY_TYPE_USER},
        {{OSP_FLAT, OSP_LINEAR}, RTC_GEOMETRY_TYPE_FLAT_LINEAR_CURVE},
        {{OSP_RIBBON, OSP_LINEAR}, (RTCGeometryType)-1},

        {{OSP_ROUND, OSP_BEZIER}, RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE},
        {{OSP_FLAT, OSP_BEZIER}, RTC_GEOMETRY_TYPE_FLAT_BEZIER_CURVE},
        {{OSP_RIBBON, OSP_BEZIER},
            RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BEZIER_CURVE},

        {{OSP_ROUND, OSP_BSPLINE}, RTC_GEOMETRY_TYPE_ROUND_BSPLINE_CURVE},
        {{OSP_FLAT, OSP_BSPLINE}, RTC_GEOMETRY_TYPE_FLAT_BSPLINE_CURVE},
        {{OSP_RIBBON, OSP_BSPLINE},
            RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_BSPLINE_CURVE},

        {{OSP_ROUND, OSP_HERMITE}, RTC_GEOMETRY_TYPE_ROUND_HERMITE_CURVE},
        {{OSP_FLAT, OSP_HERMITE}, RTC_GEOMETRY_TYPE_FLAT_HERMITE_CURVE},
        {{OSP_RIBBON, OSP_HERMITE},
            RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_HERMITE_CURVE},

        {{OSP_ROUND, OSP_CATMULL_ROM},
            RTC_GEOMETRY_TYPE_ROUND_CATMULL_ROM_CURVE},
        {{OSP_FLAT, OSP_CATMULL_ROM}, RTC_GEOMETRY_TYPE_FLAT_CATMULL_ROM_CURVE},
        {{OSP_RIBBON, OSP_CATMULL_ROM},
            RTC_GEOMETRY_TYPE_NORMAL_ORIENTED_CATMULL_ROM_CURVE}};

// Curves definitions ///////////////////////////////////////////////////////

Curves::Curves()
{
  ispcEquivalent = ispc::Curves_create(this);
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
  vertexData = getParamDataT<vec3f>("vertex.position");
  texcoordData = getParamDataT<vec2f>("vertex.texcoord");

  if (vertexData) { // round, linear curves with constant radius
    radius = getParam<float>("radius", 0.01f);
    curveType = (OSPCurveType)getParam<int>("type", OSP_ROUND);
    curveBasis = (OSPCurveBasis)getParam<int>("basis", OSP_LINEAR);
    if (curveMap[std::make_pair(curveType, curveBasis)]
        != RTC_GEOMETRY_TYPE_USER)
      throw std::runtime_error(
          "constant-radius curves need to be of type OSP_ROUND and have "
          "basis OSP_LINEAR");
  } else { // embree curves
    vertexData = getParamDataT<vec4f>("vertex.position_radius", true);
    radius = 0.0f;

    curveType = (OSPCurveType)getParam<int>("type", OSP_UNKNOWN_CURVE_TYPE);
    if (curveType == OSP_UNKNOWN_CURVE_TYPE)
      throw std::runtime_error("curves geometry has invalid 'type'");

    curveBasis = (OSPCurveBasis)getParam<int>("basis", OSP_UNKNOWN_CURVE_BASIS);

    if (curveBasis == OSP_UNKNOWN_CURVE_BASIS)
      throw std::runtime_error("curves geometry has invalid 'basis'");

    if (curveBasis == OSP_LINEAR && curveType != OSP_FLAT) {
      throw std::runtime_error(
          "curves geometry with linear basis must be of flat type or have "
          "constant radius");
    }

    if (curveType == OSP_RIBBON)
      normalData = getParamDataT<vec3f>("vertex.normal", true);

    if (curveBasis == OSP_HERMITE)
      tangentData = getParamDataT<vec4f>("vertex.tangent", true);
  }

  colorData = getParamDataT<vec4f>("vertex.color");

  embreeCurveType = curveMap[std::make_pair(curveType, curveBasis)];

  createEmbreeGeometry();

  postCreationInfo(vertexData->size());
}

size_t Curves::numPrimitives() const
{
  return indexData->size();
}

void Curves::createEmbreeGeometry()
{
  if (embreeGeometry)
    rtcReleaseGeometry(embreeGeometry);

  embreeGeometry = rtcNewGeometry(ispc_embreeDevice(), embreeCurveType);

  if (embreeCurveType == RTC_GEOMETRY_TYPE_USER) {
    ispc::Curves_setUserGeometry(getIE(),
        embreeGeometry,
        radius,
        ispc(indexData),
        ispc(vertexData),
        ispc(colorData),
        ispc(texcoordData));
  } else {
    Ref<const DataT<vec4f>> vertex4f(&vertexData->as<vec4f>());
    setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX, vertex4f);
    setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_INDEX, indexData);
    setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_NORMAL, normalData);
    setEmbreeGeometryBuffer(
        embreeGeometry, RTC_BUFFER_TYPE_TANGENT, tangentData);
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

    ispc::Curves_set(
        getIE(), embreeGeometry, colorData, texcoordData, indexData->size());

    rtcCommitGeometry(embreeGeometry);
  }
}

} // namespace ospray
