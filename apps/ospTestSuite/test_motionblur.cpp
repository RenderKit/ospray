// Copyright 2017-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ospray_testing.h"
#include "test_fixture.h"

namespace OSPRayTestScenes {

class MotionBlurBoxes
    : public Base,
      public ::testing::TestWithParam<const char * /*renderer*/>
{
 public:
  MotionBlurBoxes();
  void SetUp() override;
};

MotionBlurBoxes::MotionBlurBoxes()
{
  rendererType = GetParam();
  samplesPerPixel = 64;
}

void MotionBlurBoxes::SetUp()
{
  Base::SetUp();
  renderer.setParam("backgroundColor", vec4f(0.2, 0.2, 0.2, 1.0f));
  camera.setParam("position", vec3f(0, 0, -9));
  camera.setParam("shutter", range1f(0.0f, 1.0f));

  cpp::Geometry boxGeometry("box");
  boxGeometry.setParam(
      "box", cpp::CopiedData(box3f(vec3f(-0.5f), vec3f(0.5f))));
  boxGeometry.commit();
  cpp::GeometricModel model(boxGeometry);

  cpp::Material material(rendererType, "obj");
  material.setParam("kd", vec3f(0.8f, 0.1, 0.4));
  material.setParam("ks", vec3f(0.2f));
  material.setParam("ns", 99.f);
  material.commit();
  model.setParam("material", material);
  model.commit();

  cpp::Volume volume("structuredRegular");
  std::vector<float> volData(8);
  std::generate(
      volData.begin(), volData.end(), [n = 0]() mutable { return n++; });
  volume.setParam("data", cpp::CopiedData(volData.data(), vec3ul(2)));
  volume.setParam("gridOrigin", vec3f(-0.5, 0.5, -0.5));
  volume.setParam("filter", (int)OSP_VOLUME_FILTER_NEAREST);
  volume.commit();

  cpp::TransferFunction transferFun("piecewiseLinear");
  transferFun.setParam("valueRange", vec2f(0.f, 7.f));
  std::vector<vec3f> colors = {vec3f(1.0f, 0.5f, 0.5f),
      vec3f(0.5f, 1.0f, 0.5f),
      vec3f(0.5f, 1.0f, 1.0f),
      vec3f(0.5f, 0.5f, 1.0f)};
  std::vector<float> opacities = {0.1f, 1.0f};
  transferFun.setParam("color", cpp::CopiedData(colors));
  transferFun.setParam("opacity", cpp::CopiedData(opacities));
  transferFun.commit();

  cpp::VolumetricModel volModel(volume);
  volModel.setParam("transferFunction", transferFun);
  volModel.setParam("densityScale", 99.0f);
  volModel.commit();

  cpp::Geometry sphereGeometry("sphere");
  sphereGeometry.setParam(
      "sphere.position", cpp::CopiedData(vec3f(0.3f, 0.6f, -0.2f)));
  sphereGeometry.setParam("radius", 0.6f);
  sphereGeometry.commit();
  cpp::GeometricModel clipModel(sphereGeometry);
  clipModel.commit();

  cpp::Group group;
  group.setParam("geometry", cpp::CopiedData(model));
  group.setParam("volume", cpp::CopiedData(volModel));
  group.setParam("clippingGeometry", cpp::CopiedData(clipModel));
  group.commit();

  { // static original
    cpp::Instance instance(group);
    instance.setParam("transform", affine3f::rotate(vec3f(1, 1, 0.1), -0.4));
    AddInstance(instance);
  }

  { // linear
    cpp::Instance instance(group);
    std::vector<affine3f> xfms;
    xfms.push_back(affine3f::translate(vec3f(3, -3, 0)));
    xfms.push_back(affine3f::translate(vec3f(2.5, -3, 0)));
    xfms.push_back(affine3f::translate(vec3f(2.2, -1, 0)));
    instance.setParam("motion.transform", cpp::CopiedData(xfms));
    AddInstance(instance);
  }

  { // linear, time
    cpp::Instance instance(group);
    std::vector<affine3f> xfms;
    xfms.push_back(affine3f::translate(vec3f(5, -3, 0)));
    xfms.push_back(affine3f::translate(vec3f(4.5, -3, 0)));
    xfms.push_back(affine3f::translate(vec3f(4.2, -1, 0)));
    instance.setParam("motion.transform", cpp::CopiedData(xfms));
    instance.setParam("time", range1f(-2, 0.8));
    AddInstance(instance);
  }

  { // scale
    cpp::Instance instance(group);
    std::vector<affine3f> xfms;
    xfms.push_back(affine3f::translate(vec3f(1, 2, 0)));
    xfms.push_back(affine3f::translate(vec3f(1, 2, 0))
        * affine3f::scale(vec3f(1.8, 0.9, 1)));
    instance.setParam("motion.transform", cpp::CopiedData(xfms));
    AddInstance(instance);
  }

  { // rot
    cpp::Instance instance(group);
    std::vector<affine3f> xfms;
    xfms.push_back(affine3f::rotate(vec3f(0, 4, 0), vec3f(0, 0, 1), 0.8));
    xfms.push_back(affine3f::rotate(vec3f(0, 4, 0), vec3f(0, 0, 1), 1.2));
    xfms.push_back(affine3f::rotate(vec3f(0, 4, 0), vec3f(0, 0, 1), 1.6));
    instance.setParam("motion.transform", cpp::CopiedData(xfms));
    AddInstance(instance);
  }

  cpp::Light distant("distant");
  distant.setParam("intensity", 3.0f);
  distant.setParam("direction", vec3f(0.3f, -4.0f, 2.8f));
  distant.setParam("color", vec3f(1.0f, 0.5f, 0.5f));
  distant.setParam("angularDiameter", 1.0f);
  AddLight(distant);

  cpp::Light ambient("ambient");
  ambient.setParam("visible", false);
  ambient.setParam("intensity", 0.1f);
  AddLight(ambient);
}

// Test camera MB (also in combination with stereo) ////////////////////////////
class MotionCamera : public Base,
                     public ::testing::TestWithParam<
                         std::tuple<const char * /*camera*/, OSPStereoMode>>
{
 public:
  MotionCamera();
  void SetUp() override;

 protected:
  std::string cameraType;
  vec3f pos{0.0f, 0.0f, -0.5f};
  OSPStereoMode stereoMode;
};

MotionCamera::MotionCamera()
{
  rendererType = "pathtracer";
  samplesPerPixel = 64;

  auto params = GetParam();
  cameraType = std::get<0>(params);
  stereoMode = std::get<1>(params);
}

void MotionCamera::SetUp()
{
  Base::SetUp();

  instances.clear();

  auto builder = ospray::testing::newBuilder("cornell_box");
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);

  world = ospray::testing::buildWorld(builder);
  ospray::testing::release(builder);
  world.commit();

  camera = cpp::Camera(cameraType);
  if (cameraType != "panoramic") {
    pos.z = -2.f;
    camera.setParam("aspect", imgSize.x / (float)imgSize.y);
  }
  camera.setParam("position", pos);
  if (cameraType == "orthographic")
    camera.setParam("height", 2.0f);
  else
    camera.setParam("stereoMode", stereoMode);

  std::vector<affine3f> xfms;
  xfms.push_back(affine3f::rotate(vec3f(0, 4, 0), vec3f(0, 0, 1), 0.1));
  xfms.push_back(affine3f::rotate(vec3f(0, 4, 0), vec3f(0, 0, 1), 0.12));
  camera.setParam("motion.transform", cpp::CopiedData(xfms));
  camera.setParam("shutter", range1f(0.0f, 1.0f));
  camera.commit();
}

// Test Instantiations //////////////////////////////////////////////////////

TEST_P(MotionBlurBoxes, instance_mb)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(
    TestMotionBlur, MotionBlurBoxes, ::testing::Values("scivis", "pathtracer"));

TEST_P(MotionCamera, camera_mb)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Camera,
    MotionCamera,
    ::testing::Combine(::testing::Values("perspective", "panoramic"),
        ::testing::Values(
            OSP_STEREO_NONE, OSP_STEREO_RIGHT, OSP_STEREO_SIDE_BY_SIDE)));

INSTANTIATE_TEST_SUITE_P(CameraOrtho,
    MotionCamera,
    ::testing::Values(std::make_tuple("orthographic", OSP_STEREO_NONE)));

} // namespace OSPRayTestScenes
