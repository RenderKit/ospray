// Copyright 2009-2021 Intel Corporation
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
  GeometricModel(const Geometry &);
  GeometricModel(OSPGeometry);
  GeometricModel(OSPGeometricModel existing = nullptr);
};

static_assert(sizeof(GeometricModel) == sizeof(OSPGeometricModel),
    "cpp::GeometricModel can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline GeometricModel::GeometricModel(const Geometry &geom)
    : GeometricModel(geom.handle())
{}

inline GeometricModel::GeometricModel(OSPGeometry geom)
{
  ospObject = ospNewGeometricModel(geom);
}

inline GeometricModel::GeometricModel(OSPGeometricModel existing)
    : ManagedObject<OSPGeometricModel, OSP_GEOMETRIC_MODEL>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::GeometricModel, OSP_GEOMETRIC_MODEL);

} // namespace ospray
