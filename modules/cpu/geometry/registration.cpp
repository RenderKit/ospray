// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Boxes.h"
#include "Curves.h"
#ifdef OSPRAY_ENABLE_VOLUMES
#include "Isosurfaces.h"
#endif
#include "Mesh.h"
#include "Planes.h"
#include "Spheres.h"
#ifndef OSPRAY_TARGET_SYCL
#include "Subdivision.h"
#endif

namespace ospray {

void registerAllGeometries()
{
  Geometry::registerType<Boxes>("box");
  Geometry::registerType<Curves>("curve");
#ifdef OSPRAY_ENABLE_VOLUMES
  Geometry::registerType<Isosurfaces>("isosurface");
#endif
  Geometry::registerType<Mesh>("mesh");
  Geometry::registerType<Spheres>("sphere");
#ifndef OSPRAY_TARGET_SYCL
  // Subdivision surfaces not supported on the GPU
  Geometry::registerType<Subdivision>("subdivision");
#endif
  Geometry::registerType<Planes>("plane");
}

} // namespace ospray
