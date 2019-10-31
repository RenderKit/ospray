// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
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
  }

  void SetRendererParameters(cpp::Renderer r)
  {
    r.setParam("spp", SPP);
  }
};

using CornellBox_spp_1_sv  = CornellBox<1, true>;
using CornellBox_spp_2_sv  = CornellBox<2, true>;
using CornellBox_spp_4_sv  = CornellBox<4, true>;
using CornellBox_spp_8_sv  = CornellBox<8, true>;
using CornellBox_spp_16_sv = CornellBox<16, true>;
using CornellBox_spp_32_sv = CornellBox<32, true>;

OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_1_sv);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_2_sv);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_4_sv);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_8_sv);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_16_sv);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_32_sv);

using CornellBox_spp_1_pt  = CornellBox<1, false>;
using CornellBox_spp_2_pt  = CornellBox<2, false>;
using CornellBox_spp_4_pt  = CornellBox<4, false>;
using CornellBox_spp_8_pt  = CornellBox<8, false>;
using CornellBox_spp_16_pt = CornellBox<16, false>;
using CornellBox_spp_32_pt = CornellBox<32, false>;

OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_1_pt);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_2_pt);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_4_pt);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_8_pt);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_16_pt);
OSPRAY_DEFINE_BENCHMARK(CornellBox_spp_32_pt);
