// Copyright 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../BaseFixture.h"

template <bool ADD_TORUS = true, bool USE_SCIVIS = true>
class PerlinNoiseVolumes : public BaseFixture
{
 public:
  PerlinNoiseVolumes()
      : BaseFixture(
          USE_SCIVIS ? "scivis" : "pathtracer", "gravity_spheres_volume")
  {
    outputFilename = "PerlinNoiseVolumes";
    outputFilename += "_add_torus_" + std::to_string(ADD_TORUS);
    outputFilename += "_" + rendererType;
  }

  void SetBuilderParameters(testing::SceneBuilderHandle scene) override
  {
    testing::setParam(scene, "addTorusVolume", ADD_TORUS);
  }
};

using PerlinNoiseVolumes_sphere_only_scivis = PerlinNoiseVolumes<true, true>;
using PerlinNoiseVolumes_both_volumes_scivis = PerlinNoiseVolumes<false, true>;

OSPRAY_DEFINE_BENCHMARK(PerlinNoiseVolumes_sphere_only_scivis);
OSPRAY_DEFINE_BENCHMARK(PerlinNoiseVolumes_both_volumes_scivis);

using PerlinNoiseVolumes_sphere_only_pathtracer =
    PerlinNoiseVolumes<true, false>;
using PerlinNoiseVolumes_both_volumes_pathtracer =
    PerlinNoiseVolumes<false, false>;

OSPRAY_DEFINE_BENCHMARK(PerlinNoiseVolumes_sphere_only_pathtracer);
OSPRAY_DEFINE_BENCHMARK(PerlinNoiseVolumes_both_volumes_pathtracer);
