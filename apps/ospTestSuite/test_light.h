// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

namespace OSPRayTestScenes {

class SunSky : public Base,
               public ::testing::TestWithParam<std::tuple<vec3f /*up*/,
                   vec3f /*direction*/,
                   float /*turbidity*/,
                   float /*albedo*/>>
{
 public:
  SunSky();
  void SetUp() override;
};

} // namespace OSPRayTestScenes
