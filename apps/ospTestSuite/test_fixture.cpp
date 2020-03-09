// Copyright 2017-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"
#include "ArcballCamera.h"
#include "ospray_testing.h"

extern OSPRayEnvironment *ospEnv;

namespace OSPRayTestScenes {

Base::Base()
{
  const ::testing::TestCase *const testCase =
      ::testing::UnitTest::GetInstance()->current_test_case();
  const ::testing::TestInfo *const testInfo =
      ::testing::UnitTest::GetInstance()->current_test_info();
  imgSize = ospEnv->GetImgSize();

  framebuffer = cpp::FrameBuffer(
      imgSize, frameBufferFormat, OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_DEPTH);

  {
    std::string testCaseName = testCase->name();
    std::string testInfoName = testInfo->name();
    size_t pos = testCaseName.find('/');
    if (pos == std::string::npos) {
      testName = testCaseName + "_" + testInfoName;
    } else {
      std::string instantiationName = testCaseName.substr(0, pos);
      std::string className = testCaseName.substr(pos + 1);
      testName = className + "_" + instantiationName + "_" + testInfoName;
    }
    for (char &byte : testName)
      if (byte == '/')
        byte = '_';
  }

  rendererType = "scivis";
  frames = 1;
  samplesPerPixel = 16;

  imageTool.reset(new OSPImageTools(imgSize, GetTestName(), frameBufferFormat));
}

void Base::SetUp()
{
  CreateEmptyScene();
}

void Base::AddLight(cpp::Light light)
{
  light.commit();
  lightsList.push_back(light);
}

void Base::AddModel(cpp::GeometricModel model, affine3f xfm)
{
  model.commit();

  cpp::Group group;
  group.setParam("geometry", cpp::Data(model));
  group.commit();

  cpp::Instance instance(group);
  instance.setParam("xfm", xfm);

  AddInstance(instance);
}

void Base::AddModel(cpp::VolumetricModel model, affine3f xfm)
{
  model.commit();

  cpp::Group group;
  group.setParam("volume", cpp::Data(model));
  group.commit();

  cpp::Instance instance(group);
  instance.setParam("xfm", xfm);

  AddInstance(instance);
}

void Base::AddInstance(cpp::Instance instance)
{
  instance.commit();
  instances.push_back(instance);
}

void Base::PerformRenderTest()
{
  SetLights();

  if (!instances.empty())
    world.setParam("instance", cpp::Data(instances));

  camera.commit();
  world.commit();
  renderer.commit();

  framebuffer.resetAccumulation();

  RenderFrame();

  auto *framebuffer_data = (uint32_t *)framebuffer.map(OSP_FB_COLOR);

  if (ospEnv->GetDumpImg()) {
    EXPECT_EQ(imageTool->saveTestImage(framebuffer_data), OsprayStatus::Ok);
  } else {
    EXPECT_EQ(
        imageTool->compareImgWithBaseline(framebuffer_data), OsprayStatus::Ok);
  }

  framebuffer.unmap(framebuffer_data);
}

void Base::CreateEmptyScene()
{
  camera = cpp::Camera("perspective");
  camera.setParam("aspect", imgSize.x / (float)imgSize.y);
  camera.setParam("position", vec3f(0.f));
  camera.setParam("direction", vec3f(0.f, 0.f, 1.f));
  camera.setParam("up", vec3f(0.f, 1.f, 0.f));

  renderer = cpp::Renderer(rendererType);
  if (rendererType == "scivis")
    renderer.setParam("aoSamples", 0);
  renderer.setParam("backgroundColor", vec3f(1.0f));
  renderer.setParam("pixelSamples", samplesPerPixel);

  world = cpp::World();
}

void Base::SetLights()
{
  if (!lightsList.empty())
    world.setParam("light", cpp::Data(lightsList));
}

void Base::RenderFrame()
{
  for (int frame = 0; frame < frames; ++frame)
    framebuffer.renderFrame(renderer, camera, world);
}

FromOsprayTesting::FromOsprayTesting()
{
  auto params = GetParam();

  sceneName = std::get<0>(params);
  rendererType = std::get<1>(params);
}

void FromOsprayTesting::SetUp()
{
  Base::SetUp();

  instances.clear();

  auto builder = ospray::testing::newBuilder(sceneName);
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);

  world = ospray::testing::buildWorld(builder);
  ospray::testing::release(builder);

  world.commit();

  auto worldBounds = world.getBounds();

  ArcballCamera arcballCamera(worldBounds, imgSize);

  camera.setParam("position", arcballCamera.eyePos());
  camera.setParam("direction", arcballCamera.lookDir());
  camera.setParam("up", arcballCamera.upDir());
}

} // namespace OSPRayTestScenes
