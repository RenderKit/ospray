// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../BaseFixture.h"

class GravitySpheres : public BaseFixture
{
 public:
  GravitySpheres(
      const std::string &n, const std::string &s, int d, const std::string &r)
      : BaseFixture(n + s + "/dim_" + std::to_string(d), s, r), dim(d)
  {}

  void SetBuilderParameters(testing::SceneBuilderHandle scene) override
  {
    testing::setParam(scene, "volumeDimensions", vec3i(dim));
  }

 private:
  int dim;
};

OSPRAY_DEFINE_BENCHMARK(GravitySpheres, "gravity_spheres_volume", 32, "ao");
OSPRAY_DEFINE_BENCHMARK(GravitySpheres, "gravity_spheres_volume", 128, "ao");
OSPRAY_DEFINE_BENCHMARK(GravitySpheres, "gravity_spheres_volume", 512, "ao");
OSPRAY_DEFINE_BENCHMARK(GravitySpheres, "gravity_spheres_volume", 32, "scivis");
OSPRAY_DEFINE_BENCHMARK(
    GravitySpheres, "gravity_spheres_volume", 128, "scivis");
OSPRAY_DEFINE_BENCHMARK(
    GravitySpheres, "gravity_spheres_volume", 512, "scivis");
OSPRAY_DEFINE_BENCHMARK(
    GravitySpheres, "gravity_spheres_volume", 32, "pathtracer");
OSPRAY_DEFINE_BENCHMARK(
    GravitySpheres, "gravity_spheres_volume", 128, "pathtracer");
OSPRAY_DEFINE_BENCHMARK(
    GravitySpheres, "gravity_spheres_volume", 512, "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(
    GravitySpheres, "gravity_spheres_volume", 128, "pathtracer");