// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

namespace OSPRayTestScenes {

// Test a texture colored by a volume.  Creates spheres colored by the torus
// volume It's parametrized with type of the renderer.
class TextureVolumeTransform_deprecated
    : public Base,
      public ::testing::TestWithParam<const char *>
{
 public:
  TextureVolumeTransform_deprecated();
  void SetUp() override;
};

class TextureVolumeTransform : public Base,
                               public ::testing::TestWithParam<const char *>
{
 public:
  TextureVolumeTransform();
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

// Test an unstructured volume rendering. Generates a complex scene made of
// different cell types. Parametrized with cells visibility boolean.
class UnstructuredVolume
    : public Base,
      public ::testing::TestWithParam<std::tuple<const char *, bool>>
{
 public:
  UnstructuredVolume();
  void SetUp() override;

 private:
  bool showCells;
};

} // namespace OSPRayTestScenes
