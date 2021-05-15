// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

namespace OSPRayTestScenes {

// Fixture class to test cornercases of intersection precision and epsilon
// handling; parametrized with renderer, sphere radius, distance factor, and
// whether the sphere is in origin
class SpherePrecision
    : public Base,
      public ::testing::TestWithParam<std::tuple<float /*radius*/,
          float /*factor*/,
          bool /*move_cam*/,
          const char * /*renderer*/>>
{
 public:
  SpherePrecision();
  void SetUp() override;

 protected:
  float dist;
  float radius;
  bool move_cam;
};

// Fixture class used for testing curves variants
class Curves
    : public Base,
      public ::testing::TestWithParam<std::tuple<const char * /*curve variant*/,
          const char * /*renderer type*/>>
{
 public:
  Curves();
  void SetUp() override;

 protected:
  std::string curveVariant;
};

} // namespace OSPRayTestScenes
