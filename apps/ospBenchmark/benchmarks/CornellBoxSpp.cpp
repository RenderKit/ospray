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

template <int SPP, bool USE_SCIVIS = true>
class CornellBox : public BaseFixture
{
 public:
  CornellBox()
      : BaseFixture(USE_SCIVIS ? "scivis" : "pathtracer", "cornell_box")
  {
    outputFilename = "CornellBox";
    outputFilename += "_spp_" + std::to_string(SPP);
    outputFilename += "_" + rendererType;
  }

  void SetRendererParameters(cpp::Renderer r)
  {
    r.setParam("pixelSamples", SPP);
  }
};

using CornellBox_spp_1_scivis = CornellBox<1, true>;
using CornellBox_spp_2_scivis = CornellBox<2, true>;
using CornellBox_spp_4_scivis = CornellBox<4, true>;
using CornellBox_spp_8_scivis = CornellBox<8, true>;
using CornellBox_spp_16_scivis = CornellBox<16, true>;
using CornellBox_spp_32_scivis = CornellBox<32, true>;

OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_1_scivis);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_2_scivis);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_4_scivis);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_8_scivis);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_16_scivis);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_32_scivis);

using CornellBox_spp_1_pathtracer = CornellBox<1, false>;
using CornellBox_spp_2_pathtracer = CornellBox<2, false>;
using CornellBox_spp_4_pathtracer = CornellBox<4, false>;
using CornellBox_spp_8_pathtracer = CornellBox<8, false>;
using CornellBox_spp_16_pathtracer = CornellBox<16, false>;
using CornellBox_spp_32_pathtracer = CornellBox<32, false>;

OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_1_pathtracer);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_2_pathtracer);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_4_pathtracer);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_8_pathtracer);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_16_pathtracer);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_32_pathtracer);
