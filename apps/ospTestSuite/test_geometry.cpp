// Copyright 2017-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_geometry.h"

namespace OSPRayTestScenes {

SpherePrecision::SpherePrecision()
{
  auto params = GetParam();
  radius = std::get<0>(params);
  dist = std::get<1>(params) * radius;
  move_cam = std::get<2>(params);
  rendererType = std::get<3>(params);
}

void SpherePrecision::SetUp()
{
  Base::SetUp();

  float fov = 160.0f * std::min(std::tan(radius / std::abs(dist)), 1.0f);
  float cent = move_cam ? 0.0f : dist + radius;

  camera.setParam(
      "position", vec3f(0.f, 0.f, move_cam ? -dist - radius : 0.0f));
  camera.setParam("direction", vec3f(0.f, 0.f, 1.f));
  camera.setParam("up", vec3f(0.f, 1.f, 0.f));
  camera.setParam("fovy", fov);

  renderer.setParam("pixelSamples", 16);
  renderer.setParam("backgroundColor", vec4f(0.2f, 0.2f, 0.4f, 1.0f));
  if (rendererType == "scivis") {
    renderer.setParam("aoSamples", 16);
    renderer.setParam("aoIntensity", 1.f);
  } else if (rendererType == "pathtracer") {
    renderer.setParam("maxPathLength", 2);
  }

  cpp::Geometry sphere("sphere");
  cpp::Geometry inst_sphere("sphere");

  std::vector<vec3f> sph_center = {vec3f(-0.5f * radius, -0.3f * radius, cent),
      vec3f(0.8f * radius, -0.3f * radius, cent),
      vec3f(0.8f * radius, 1.5f * radius, cent),
      vec3f(0.0f, -10001.3f * radius, cent)};

  std::vector<float> sph_radius = {
      radius, 0.9f * radius, 0.9f * radius, 10000.f * radius};

  sphere.setParam("sphere.position", cpp::Data(sph_center));
  sphere.setParam("sphere.radius", cpp::Data(sph_radius));
  sphere.commit();

  inst_sphere.setParam("sphere.position", cpp::Data(vec3f(0.f)));
  inst_sphere.setParam("sphere.radius", cpp::Data(90.f * radius));
  inst_sphere.commit();

  cpp::GeometricModel model1(sphere);
  cpp::GeometricModel model2(inst_sphere);

  cpp::Material sphereMaterial(rendererType, "obj");
  sphereMaterial.setParam("d", 1.0f);
  sphereMaterial.commit();

  model1.setParam("material", sphereMaterial);
  model2.setParam("material", sphereMaterial);

  affine3f xfm(vec3f(0.01, 0, 0),
      vec3f(0, 0.01, 0),
      vec3f(0, 0, 0.01),
      vec3f(-0.5f * radius, 1.6f * radius, cent));

  AddModel(model1);
  AddModel(model2, xfm);

  cpp::Light distant("distant");
  distant.setParam("intensity", 3.0f);
  distant.setParam("direction", vec3f(0.3f, -4.0f, 0.8f));
  distant.setParam("color", vec3f(1.0f, 0.5f, 0.5f));
  distant.setParam("angularDiameter", 1.0f);
  AddLight(distant);

  cpp::Light ambient = ospNewLight("ambient");
  ambient.setParam("intensity", 0.1f);
  AddLight(ambient);
}

// Test Instantiations //////////////////////////////////////////////////////

TEST_P(SpherePrecision, sphere)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Intersection,
    SpherePrecision,
    ::testing::Combine(::testing::Values(0.001f, 3.e5f),
        ::testing::Values(3.f, 8000.0f, 2.e5f),
        ::testing::Values(true, false),
        ::testing::Values("scivis", "pathtracer")));

TEST_P(FromOsprayTesting, test_scenes)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(TestScenesGeometry,
    FromOsprayTesting,
    ::testing::Combine(::testing::Values("cornell_box",
                           "curves",
                           "gravity_spheres_isosurface",
                           "empty",
                           "random_spheres",
                           "streamlines",
                           "subdivision_cube",
                           "cornell_box_photometric",
                           "planes"),
        ::testing::Values("scivis", "pathtracer")));

INSTANTIATE_TEST_SUITE_P(TestScenesClipping,
    FromOsprayTesting,
    ::testing::Combine(::testing::Values("clip_with_spheres",
                           "clip_with_boxes",
                           "clip_with_planes",
                           "clip_with_meshes",
                           "clip_with_subdivisions",
                           "clip_with_linear_curves",
                           "clip_with_bspline_curves",
                           "clip_gravity_spheres_volume",
                           "clip_perlin_noise_volumes"),
        ::testing::Values("scivis", "pathtracer")));
} // namespace OSPRayTestScenes
