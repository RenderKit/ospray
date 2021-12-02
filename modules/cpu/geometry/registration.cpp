// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Boxes.h"
#include "Curves.h"
#include "Isosurfaces.h"
#include "Mesh.h"
#include "Planes.h"
#include "Spheres.h"
#include "Subdivision.h"

namespace ospray {

void registerAllGeometries()
{
  Geometry::registerType<Boxes>("box");
  Geometry::registerType<Curves>("curve");
  Geometry::registerType<Isosurfaces>("isosurface");
  Geometry::registerType<Mesh>("mesh");
  Geometry::registerType<Spheres>("sphere");
  Geometry::registerType<Subdivision>("subdivision");
  Geometry::registerType<Planes>("plane");
}

} // namespace ospray
