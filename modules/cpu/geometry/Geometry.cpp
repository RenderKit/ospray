// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Geometry.h"
#include "common/Data.h"
#include "common/Util.h"

namespace ospray {

static FactoryMap<Geometry> g_geomMap;

// Geometry definitions ///////////////////////////////////////////////////////

Geometry::Geometry()
{
  managedObjectType = OSP_GEOMETRY;
}

Geometry::~Geometry()
{
  if (embreeGeometry)
    rtcReleaseGeometry(embreeGeometry);
}

std::string Geometry::toString() const
{
  return "ospray::Geometry";
}

Geometry *Geometry::createInstance(const char *type)
{
  return createInstanceHelper(type, g_geomMap[type]);
}

void Geometry::registerType(const char *type, FactoryFcn<Geometry> f)
{
  g_geomMap[type] = f;
}

void Geometry::postCreationInfo(size_t numVerts) const
{
  std::stringstream ss;
  ss << toString() << " created: #primitives=" << numPrimitives();
  if (numVerts > 0)
    ss << ", #vertices=" << numVerts;
  postStatusMsg(OSP_LOG_INFO) << ss.str();
}

void Geometry::setDevice(RTCDevice device)
{
  embreeDevice = device;
}

void Geometry::createEmbreeGeometry(RTCGeometryType type)
{
  if (embreeGeometry)
    rtcReleaseGeometry(embreeGeometry);

  if (!embreeDevice) {
    throw std::runtime_error("invalid Embree device");
  }

  embreeGeometry = rtcNewGeometry(embreeDevice, type);
}

void Geometry::createEmbreeUserGeometry(RTCBoundsFunction boundsFn,
    RTCIntersectFunctionN intersectFn,
    RTCOccludedFunctionN occludedFn)
{
  createEmbreeGeometry(RTC_GEOMETRY_TYPE_USER);

  // Setup Embree user-defined geometry
  rtcSetGeometryUserData(embreeGeometry, getSh());
  rtcSetGeometryUserPrimitiveCount(embreeGeometry, numPrimitives());
  rtcSetGeometryBoundsFunction(embreeGeometry, boundsFn, getSh());
  rtcSetGeometryIntersectFunction(embreeGeometry, intersectFn);
  rtcSetGeometryOccludedFunction(embreeGeometry, occludedFn);
  rtcCommitGeometry(embreeGeometry);
}

OSPTYPEFOR_DEFINITION(Geometry *);

} // namespace ospray
