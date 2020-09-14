// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../BaseFixture.h"

class CornellBox : public BaseFixture
{
 public:
  CornellBox(const std::string &s, int spp, const std::string &r)
      : BaseFixture(s, r), pixelSamples(spp)
  {
    SetName(s + "/spp_" + std::to_string(spp) + "/" + r);
  }

  void SetRendererParameters(cpp::Renderer r)
  {
    r.setParam("pixelSamples", pixelSamples);
  }

 private:
  int pixelSamples;
};

OSPRAY_DEFINE_BENCHMARK(CornellBox, "cornell_box", 1, "ao");
OSPRAY_DEFINE_BENCHMARK(CornellBox, "cornell_box", 16, "ao");
OSPRAY_DEFINE_BENCHMARK(CornellBox, "cornell_box", 256, "ao");
OSPRAY_DEFINE_BENCHMARK(CornellBox, "cornell_box", 1, "scivis");
OSPRAY_DEFINE_BENCHMARK(CornellBox, "cornell_box", 16, "scivis");
OSPRAY_DEFINE_BENCHMARK(CornellBox, "cornell_box", 256, "scivis");
OSPRAY_DEFINE_BENCHMARK(CornellBox, "cornell_box", 1, "pathtracer");
OSPRAY_DEFINE_BENCHMARK(CornellBox, "cornell_box", 16, "pathtracer");
OSPRAY_DEFINE_BENCHMARK(CornellBox, "cornell_box", 256, "pathtracer");
