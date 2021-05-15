// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../BaseFixture.h"

class ClippingGeometries : public BaseFixture
{
 public:
  ClippingGeometries(const std::string &n,
      const std::string &s,
      const std::string &g,
      const std::string &r)
      : BaseFixture(n + "clip_geometries/with_" + g, s, r), geometry(g)
  {}

 private:
  std::string geometry;
};

OSPRAY_DEFINE_BENCHMARK(
    ClippingGeometries, "clip_with_spheres", "spheres", "ao");
OSPRAY_DEFINE_BENCHMARK(ClippingGeometries, "clip_with_boxes", "boxes", "ao");
OSPRAY_DEFINE_BENCHMARK(ClippingGeometries, "clip_with_planes", "planes", "ao");
OSPRAY_DEFINE_BENCHMARK(ClippingGeometries, "clip_with_meshes", "meshes", "ao");

OSPRAY_DEFINE_BENCHMARK(
    ClippingGeometries, "clip_with_spheres", "spheres", "scivis");
OSPRAY_DEFINE_BENCHMARK(
    ClippingGeometries, "clip_with_boxes", "boxes", "scivis");
OSPRAY_DEFINE_BENCHMARK(
    ClippingGeometries, "clip_with_planes", "planes", "scivis");
OSPRAY_DEFINE_BENCHMARK(
    ClippingGeometries, "clip_with_meshes", "meshes", "scivis");

OSPRAY_DEFINE_BENCHMARK(
    ClippingGeometries, "clip_with_spheres", "spheres", "pathtracer");
OSPRAY_DEFINE_BENCHMARK(
    ClippingGeometries, "clip_with_boxes", "boxes", "pathtracer");
OSPRAY_DEFINE_BENCHMARK(
    ClippingGeometries, "clip_with_planes", "planes", "pathtracer");
OSPRAY_DEFINE_BENCHMARK(
    ClippingGeometries, "clip_with_meshes", "meshes", "pathtracer");

OSPRAY_DEFINE_SETUP_BENCHMARK(
    ClippingGeometries, "clip_with_spheres", "spheres", "scivis");
OSPRAY_DEFINE_SETUP_BENCHMARK(
    ClippingGeometries, "clip_with_boxes", "boxes", "scivis");
OSPRAY_DEFINE_SETUP_BENCHMARK(
    ClippingGeometries, "clip_with_planes", "planes", "scivis");
OSPRAY_DEFINE_SETUP_BENCHMARK(
    ClippingGeometries, "clip_with_meshes", "meshes", "scivis");
