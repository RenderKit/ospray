// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"
#include "Material.h"

namespace ospray {
namespace cpp {

class GeometricModel
    : public ManagedObject<OSPGeometricModel, OSP_GEOMETRIC_MODEL>
{
 public:
  GeometricModel(const Geometry &geom);
  GeometricModel(OSPGeometry geom);
  GeometricModel(const GeometricModel &copy);
  GeometricModel(OSPGeometricModel existing = nullptr);
};

static_assert(sizeof(GeometricModel) == sizeof(OSPGeometricModel),
    "cpp::GeometricModel can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline GeometricModel::GeometricModel(const Geometry &geom)
    : GeometricModel(geom.handle())
{}

inline GeometricModel::GeometricModel(OSPGeometry existing)
{
  ospObject = ospNewGeometricModel(existing);
}

inline GeometricModel::GeometricModel(const GeometricModel &copy)
    : ManagedObject<OSPGeometricModel, OSP_GEOMETRIC_MODEL>(copy.handle())
{
  ospRetain(copy.handle());
}

inline GeometricModel::GeometricModel(OSPGeometricModel existing)
    : ManagedObject<OSPGeometricModel, OSP_GEOMETRIC_MODEL>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::GeometricModel, OSP_GEOMETRIC_MODEL);

} // namespace ospray
