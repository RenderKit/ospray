// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Boxes.h"
#include "Curves.h"
// TODO: No iterators in SYCL API yet
#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
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
// TODO: No iterators in SYCL API yet
#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
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
