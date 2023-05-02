// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Geometry.h"
#include "common/Data.h"
#ifndef OSPRAY_TARGET_SYCL
#include "geometry/GeometryDispatch_ispc.h"
#endif

namespace ospray {

// Geometry definitions ///////////////////////////////////////////////////////

Geometry::Geometry(api::ISPCDevice &device, const FeatureFlagsGeometry ffg)
    : AddStructShared(device.getIspcrtContext(), device),
      featureFlagsGeometry(ffg)
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

void Geometry::postCreationInfo(size_t numVerts) const
{
  std::stringstream ss;
  ss << toString() << " created: #primitives=" << numPrimitives();
  if (numVerts > 0)
    ss << ", #vertices=" << numVerts;
  postStatusMsg(OSP_LOG_INFO) << ss.str();
}

void Geometry::createEmbreeGeometry(RTCGeometryType type)
{
  if (embreeGeometry)
    rtcReleaseGeometry(embreeGeometry);

  if (!getISPCDevice().getEmbreeDevice()) {
    throw std::runtime_error("invalid Embree device");
  }

  embreeGeometry = rtcNewGeometry(getISPCDevice().getEmbreeDevice(), type);
}

void Geometry::createEmbreeUserGeometry(RTCBoundsFunction boundsFn)
{
  createEmbreeGeometry(RTC_GEOMETRY_TYPE_USER);

  // Setup Embree user-defined geometry
  rtcSetGeometryUserData(embreeGeometry, getSh());
  rtcSetGeometryUserPrimitiveCount(embreeGeometry, numPrimitives());
  rtcSetGeometryBoundsFunction(embreeGeometry, boundsFn, getSh());
  rtcCommitGeometry(embreeGeometry);
}

OSPTYPEFOR_DEFINITION(Geometry *);

} // namespace ospray
