// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

namespace OSPRayTestScenes {

class LightTest : public Base
{
 public:
  LightTest();
  void SetUp() override;
};

class SunSky : public LightTest,
               public ::testing::TestWithParam<std::tuple<vec3f /*up*/,
                   vec3f /*direction*/,
                   float /*turbidity*/,
                   float /*albedo*/,
                   float /*"horizonExtension*/>>
{
 public:
  SunSky();
  void SetUp() override;
};

class AmbientLight
    : public LightTest,
      public ::testing::TestWithParam<const char * /*renderer type*/>
{
 public:
  AmbientLight();
  void SetUp() override;
};

class DistantLight
    : public LightTest,
      public ::testing::TestWithParam<
          std::tuple<vec3f /* direction*/, const char * /*renderer type*/>>
{
 public:
  DistantLight();
  void SetUp() override;

 private:
  vec3f direction{0.f, 0.f, 1.f};
};

class GeometricLight : public LightTest,
                       public ::testing::TestWithParam<
                           std::tuple<float /*size*/, bool /*useMaterialList*/>>
{
 public:
  GeometricLight();
  void SetUp() override;

 private:
  float size{0.2};
  bool useMaterialList{true};
};

class PhotometricLight
    : public LightTest,
      public ::testing::TestWithParam<
          std::tuple<float /*radius*/, const char * /*renderer type*/>>
{
 public:
  PhotometricLight();
  void SetUp() override;

 private:
  float radius{0.0};
};

class QuadLight
    : public LightTest,
      public ::testing::TestWithParam<
          std::tuple<float /*size*/, const char * /*renderer type*/>>
{
 public:
  QuadLight();
  void SetUp() override;

 private:
  float size{0.2};
};

class SphereLight
    : public LightTest,
      public ::testing::TestWithParam<
          std::tuple<float /*radius*/, const char * /*renderer type*/>>
{
 public:
  SphereLight();
  void SetUp() override;

 private:
  float radius{0.0};
};

class SpotLight
    : public LightTest,
      public ::testing::TestWithParam<std::tuple<vec2f /*innerOuterRadius*/,
          const char * /*renderer type*/>>
{
 public:
  SpotLight();
  void SetUp() override;

 private:
  vec2f innerOuterRadius{0.0, 0.2};
};

} // namespace OSPRayTestScenes
