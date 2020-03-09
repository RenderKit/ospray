// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

namespace OSPRayTestScenes {

// Test a texture colored by a volume.  Creates a sphere colored by the torus
// volume It's parametrized with type of the renderer.
class TextureVolume : public Base, public ::testing::TestWithParam<const char *>
{
 public:
  TextureVolume();
  void SetUp() override;
};

// Test a texture colored by a volume.  Creates a sphere colored by the torus
// volume It's parametrized with type of the renderer and background color
class DepthCompositeVolume
    : public Base,
      public ::testing::TestWithParam<std::tuple<const char *, vec4f>>
{
 public:
  DepthCompositeVolume();
  void SetUp() override;

 private:
  vec4f bgColor;
};

} // namespace OSPRayTestScenes
