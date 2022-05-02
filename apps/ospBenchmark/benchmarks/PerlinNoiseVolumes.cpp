// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../BaseFixture.h"

class PerlinNoiseVolumes : public BaseFixture
{
 public:
  PerlinNoiseVolumes(
      const std::string &n, const std::string &s, bool at, const std::string &r)
      : BaseFixture(n + s + (at ? "/both_volumes" : "/sphere_only"), s, r),
        addTorus(at)
  {}

  void SetBuilderParameters(testing::SceneBuilderHandle scene) override
  {
    testing::setParam(scene, "addTorusVolume", addTorus);
  }

 private:
  bool addTorus;
};

OSPRAY_DEFINE_BENCHMARK(
    PerlinNoiseVolumes, "perlin_noise_volumes", false, "ao");
OSPRAY_DEFINE_BENCHMARK(PerlinNoiseVolumes, "perlin_noise_volumes", true, "ao");
OSPRAY_DEFINE_BENCHMARK(
    PerlinNoiseVolumes, "perlin_noise_volumes", false, "scivis");
OSPRAY_DEFINE_BENCHMARK(
    PerlinNoiseVolumes, "perlin_noise_volumes", true, "scivis");
OSPRAY_DEFINE_BENCHMARK(
    PerlinNoiseVolumes, "perlin_noise_volumes", false, "pathtracer");
OSPRAY_DEFINE_BENCHMARK(
    PerlinNoiseVolumes, "perlin_noise_volumes", true, "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(
    PerlinNoiseVolumes, "perlin_noise_volumes", true, "pathtracer");
