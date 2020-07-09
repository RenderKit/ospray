// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../BaseFixture.h"

class Boxes : public BaseFixture
{
 public:
  Boxes(const std::string &s, int ao)
      : BaseFixture(s, "scivis"), aoSamples(ao)
  {
    SetName(s + "/ao_" + std::to_string(ao) + "/scivis");
  }

  void SetRendererParameters(cpp::Renderer r)
  {
    r.setParam("aoSamples", aoSamples);
  }

 private:
  int aoSamples;
};

OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", 0);
OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", 1);
OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", 16);
OSPRAY_DEFINE_BENCHMARK(Boxes, "boxes", 256);
