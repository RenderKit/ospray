// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

namespace OSPRayTestScenes {

// Test all texture image formats (and filter modes)
class Texture2D : public Base,
                  public ::testing::TestWithParam<
                      std::tuple<int /*filter*/, bool /*lightset*/>>
{
 public:
  Texture2D();
  void SetUp() override;
};

class Texture2DTransform : public Base,
                           public ::testing::TestWithParam<const char *>
{
 public:
  Texture2DTransform();
  void SetUp() override;
};

class RendererMaterialList : public Base,
                             public ::testing::TestWithParam<const char *>
{
 public:
  RendererMaterialList();
  void SetUp() override;
};

class PTBackgroundRefraction : public Base,
                               public ::testing::TestWithParam<bool>
{
 public:
  PTBackgroundRefraction();
  void SetUp() override;
};

} // namespace OSPRayTestScenes
