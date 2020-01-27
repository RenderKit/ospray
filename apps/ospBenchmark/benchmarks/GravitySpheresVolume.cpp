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

template <int DIM, bool USE_SCIVIS = true>
class GravitySpheres : public BaseFixture
{
 public:
  GravitySpheres()
      : BaseFixture(
          USE_SCIVIS ? "scivis" : "pathtracer", "gravity_spheres_volume")
  {
    outputFilename = "GravitySpheres";
    outputFilename += "_dim_" + std::to_string(DIM);
    outputFilename += "_" + rendererType;
  }

  void SetBuilderParameters(testing::SceneBuilderHandle scene) override
  {
    testing::setParam(scene, "volumeDimensions", vec3i(DIM));
  }
};

using GravitySpheres_dim_32_scivis = GravitySpheres<32, true>;
using GravitySpheres_dim_64_scivis = GravitySpheres<64, true>;
using GravitySpheres_dim_128_scivis = GravitySpheres<128, true>;
using GravitySpheres_dim_256_scivis = GravitySpheres<256, true>;
using GravitySpheres_dim_512_scivis = GravitySpheres<512, true>;

OSPRAY_DEFINE_BENCHMARK(GravitySpheres_dim_32_scivis);
OSPRAY_DEFINE_BENCHMARK(GravitySpheres_dim_64_scivis);
OSPRAY_DEFINE_BENCHMARK(GravitySpheres_dim_128_scivis);
OSPRAY_DEFINE_BENCHMARK(GravitySpheres_dim_256_scivis);
OSPRAY_DEFINE_BENCHMARK(GravitySpheres_dim_512_scivis);

using GravitySpheres_dim_32_pathtracer = GravitySpheres<32, false>;
using GravitySpheres_dim_64_pathtracer = GravitySpheres<64, false>;
using GravitySpheres_dim_128_pathtracer = GravitySpheres<128, false>;
using GravitySpheres_dim_256_pathtracer = GravitySpheres<256, false>;
using GravitySpheres_dim_512_pathtracer = GravitySpheres<512, false>;

OSPRAY_DEFINE_BENCHMARK(GravitySpheres_dim_32_pathtracer);
OSPRAY_DEFINE_BENCHMARK(GravitySpheres_dim_64_pathtracer);
OSPRAY_DEFINE_BENCHMARK(GravitySpheres_dim_128_pathtracer);
OSPRAY_DEFINE_BENCHMARK(GravitySpheres_dim_256_pathtracer);
OSPRAY_DEFINE_BENCHMARK(GravitySpheres_dim_512_pathtracer);
