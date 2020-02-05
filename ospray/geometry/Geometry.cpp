// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Geometry.h"
#include "common/Data.h"
#include "common/Util.h"

namespace ospray {

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
  return createInstanceHelper<Geometry, OSP_GEOMETRY>(type);
}

void Geometry::postCreationInfo(size_t numVerts) const
{
  std::stringstream ss;
  ss << toString() << " created: #primitives=" << numPrimitives();
  if (numVerts > 0)
    ss << ", #vertices=" << numVerts;
  postStatusMsg(OSP_LOG_INFO) << ss.str();
}

OSPTYPEFOR_DEFINITION(Geometry *);

} // namespace ospray
