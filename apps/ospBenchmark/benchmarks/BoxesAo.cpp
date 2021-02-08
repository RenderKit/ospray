// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../BaseFixture.h"

class Boxes : public BaseFixture
{
 public:
  Boxes(
      const std::string &n, const std::string &s, const std::string &r, int ao)
      : BaseFixture(n + s + "/ao_" + std::to_string(ao), s, r), aoSamples(ao)
  {}

  void SetRendererParameters(cpp::Renderer r) override
  {
    r.setParam("aoSamples", aoSamples);
  }

 private:
  int aoSamples;
};

OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", "ao", 0);
OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", "ao", 1);
OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", "ao", 16);
OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", "ao", 256);
OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", "scivis", 0);
OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", "scivis", 1);
OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", "scivis", 16);
OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", "scivis", 256);
OSPRAY_DEFINE_SETUP_BENCHMARK(Boxes, "boxes", "ao", 256);
