// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

namespace OSPRayTestScenes {

class LightTest : public Base
{
 public:
  LightTest();
  void SetUp() override;
  void AddInstancedLightWithMB(
      cpp::Light light, const affine3f &xfm1, const affine3f &xfm2);
  void AddInstancedLightWithTranslateMB(cpp::Light light)
  {
    AddInstancedLightWithMB(light,
        affine3f::translate(vec3f(-0.5, 0, 0)),
        affine3f::translate(vec3f(0.5, 0, 0)));
  };
  void AddInstancedLightWithRotateMB(cpp::Light light)
  {
    AddInstancedLightWithMB(light,
        affine3f::rotate(vec3f(0.f, 0.f, 1.f), -.25f * float(pi)),
        affine3f::rotate(vec3f(0.f, 0.f, 1.f), .25f * float(pi)));
  };

 protected:
  affine3f xfm;
  bool motionBlur{false};
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
      public ::testing::TestWithParam<std::tuple<vec3f /* direction*/,
          const char * /*renderer type*/,
          bool /*motionBlur*/>>
{
 public:
  DistantLight();
  void SetUp() override;

 private:
  vec3f direction{0.f, 0.f, 1.f};
};

class GeometricLight
    : public LightTest,
      public ::testing::TestWithParam<std::tuple<float /*size*/,
          bool /*useMaterialList*/,
          int /*motionBlur 1=instance, 2=deformation*/>>
{
 public:
  GeometricLight();
  void SetUp() override;

 private:
  float size{0.2};
  bool useMaterialList{true};
  int motionBlur{0};
};

class PhotometricLight
    : public LightTest,
      public ::testing::TestWithParam<std::tuple<const char * /*light type*/,
          float /*size*/,
          const char * /*renderer type*/>>
{
 public:
  PhotometricLight();
  void SetUp() override;

 private:
  std::string lightType;
  float size{0.0};
};

class QuadLight : public LightTest,
                  public ::testing::TestWithParam<std::tuple<float /*size*/,
                      const char * /*renderer type*/,
                      OSPIntensityQuantity,
                      bool /*motionBlur*/>>
{
 public:
  QuadLight();
  void SetUp() override;

 private:
  float size{0.2};
  OSPIntensityQuantity intensityQuantity{OSP_INTENSITY_QUANTITY_UNKNOWN};
};

class CylinderLight : public LightTest,
                      public ::testing::TestWithParam<std::tuple<float /*size*/,
                          const char * /*renderer type*/,
                          OSPIntensityQuantity,
                          bool /*motionBlur*/>>
{
 public:
  CylinderLight();
  void SetUp() override;

 private:
  float size{0.2};
  OSPIntensityQuantity intensityQuantity{OSP_INTENSITY_QUANTITY_UNKNOWN};
};

class SphereLight : public LightTest,
                    public ::testing::TestWithParam<std::tuple<float /*radius*/,
                        const char * /*renderer type*/,
                        OSPIntensityQuantity,
                        bool /*motionBlur*/>>
{
 public:
  SphereLight();
  void SetUp() override;

 private:
  float radius{0.0};
  OSPIntensityQuantity intensityQuantity{OSP_INTENSITY_QUANTITY_UNKNOWN};
};

class SpotLight
    : public LightTest,
      public ::testing::TestWithParam<std::tuple<vec2f /*innerOuterRadius*/,
          const char * /*renderer type*/,
          OSPIntensityQuantity,
          bool /*motionBlur*/>>
{
 public:
  SpotLight();
  void SetUp() override;

 private:
  vec2f innerOuterRadius{0.0, 0.2};
  OSPIntensityQuantity intensityQuantity{OSP_INTENSITY_QUANTITY_UNKNOWN};
};

class HDRILight
    : public LightTest,
      public ::testing::TestWithParam<
          std::tuple<const char * /*renderer type*/, bool /*motionBlur*/>>
{
 public:
  HDRILight();
  void SetUp() override;
};

class SunSky : public LightTest,
               public ::testing::TestWithParam<std::tuple<vec3f /*up*/,
                   vec3f /*direction*/,
                   float /*turbidity*/,
                   float /*albedo*/,
                   float /*horizonExtension*/,
                   bool /*motionBlur*/>>
{
 public:
  SunSky();
  void SetUp() override;
};

} // namespace OSPRayTestScenes
