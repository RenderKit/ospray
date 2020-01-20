// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file excepathtracer in compliance with the License. //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
