// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

namespace OSPRayTestScenes {

// Fixture class for a unit test of the pathtracer's shadowCatcherPlane feature
class ShadowCatcher : public Base,
                      public ::testing::TestWithParam<bool /*multipleLights*/>
{
 public:
  ShadowCatcher();
  void SetUp() override;

 protected:
};

ShadowCatcher::ShadowCatcher()
{
  rendererType = "pathtracer";
  samplesPerPixel = 32;
}

void ShadowCatcher::SetUp()
{
  Base::SetUp();
  camera.setParam("position", vec3f(-0.2f, 2.5f, -3.f));
  camera.setParam("direction", vec3f(0.f, -0.3f, 1.0f));

  cpp::Geometry boxGeometry("box");
  boxGeometry.setParam("box", cpp::CopiedData(box3f(vec3f(0.f), vec3f(0.5f, 2.f, 1.0f))));
  boxGeometry.commit();
  cpp::GeometricModel model(boxGeometry);

  cpp::Material material("obj");
  material.setParam("kd", vec3f(0.1f));
  material.commit();
  model.setParam("material", material);
  AddModel(model);

  // floor, to be hidden by ShadowCatcher
  cpp::Geometry planeGeometry("plane");
  planeGeometry.setParam("plane.coefficients", cpp::CopiedData(vec4f(0.0, 1.0, 0.0, 0.f)));
  planeGeometry.commit();
  cpp::GeometricModel planeModel(planeGeometry);

  cpp::Material planeMaterial("obj");
  planeModel.setParam("material", planeMaterial);
  AddModel(planeModel);

  if (GetParam()) {
    cpp::Light distant("distant");
    distant.setParam("direction", vec3f(5.f, -2.0f, 5.8f));
    distant.setParam("color", vec3f(15.0f, 3.0f, 3.0f));
    distant.setParam("angularDiameter", 3.0f);
    AddLight(distant);

    cpp::Light sphere("sphere");
    sphere.setParam("position", vec3f(2.f, 2.4f, -1.0f));
    sphere.setParam("color", vec3f(3.f, 150.f, 3.f));
    sphere.setParam("radius", 0.02f);
    AddLight(sphere);

    cpp::Light ambient("ambient");
    ambient.setParam("visible", false);
    ambient.setParam("color", vec3f(0.0f, 0.0f, 3.0f));
    AddLight(ambient);
  } else {
    cpp::Light light("spot");
    light.setParam("position", vec3f(-1.f, 2.4f, 1.0f));
    light.setParam("direction", vec3f(1.5f, -1.0f, 0.0f));
    light.setParam("radius", 0.2f);
    AddLight(light);
  }

  renderer.setParam("shadowCatcherPlane", vec4f(0.0, 1.0, 0.0, 0.1f));
  renderer.setParam("lightSamples", 1);
  renderer.setParam("backgroundColor", vec4f(0.3f, 0.3f, 0.3f, 0.0f));
}

TEST_P(ShadowCatcher, multipleLights)
{
  PerformRenderTest();
};

INSTANTIATE_TEST_SUITE_P(TestShadowCatcher, ShadowCatcher, ::testing::Bool());

} // namespace OSPRayTestScenes
