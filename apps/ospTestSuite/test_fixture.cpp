// Copyright 2017 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"
#include "ArcballCamera.h"
#include "ospray_testing.h"

extern OSPRayEnvironment *ospEnv;

namespace OSPRayTestScenes {

Base::Base()
{
  testName = ::testing::UnitTest::GetInstance()->current_test_case()->name();
  testName += ".";
  testName += ::testing::UnitTest::GetInstance()->current_test_info()->name();
  for (char &byte : testName)
    if (byte == '/')
      byte = '_';

  imgSize = ospEnv->GetImgSize();
  rendererType = "scivis";
  frames = 1;
  samplesPerPixel = 16;
}

void Base::SetUp()
{
  framebuffer = cpp::FrameBuffer(imgSize.x,
      imgSize.y,
      frameBufferFormat,
      OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_DEPTH);

  imageTool.reset(new OSPImageTools(imgSize, GetTestName(), frameBufferFormat));

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
  group.setParam("geometry", cpp::CopiedData(model));
  group.commit();

  cpp::Instance instance(group);
  instance.setParam("transform", xfm);

  AddInstance(instance);
}

void Base::AddModel(cpp::VolumetricModel model, affine3f xfm)
{
  model.commit();

  cpp::Group group;
  group.setParam("volume", cpp::CopiedData(model));
  group.commit();

  cpp::Instance instance(group);
  instance.setParam("transform", xfm);

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
    world.setParam("instance", cpp::CopiedData(instances));

  camera.commit();
  world.commit();
  renderer.commit();

  framebuffer.resetAccumulation();

  RenderFrame();

  void *framebuffer_data = framebuffer.map(OSP_FB_COLOR);

  if (ospEnv->GetDumpImg()) {
    EXPECT_TRUE(imageTool->saveTestImage(framebuffer_data) == OsprayStatus::Ok);
  } else {
    EXPECT_TRUE(imageTool->compareImgWithBaseline(framebuffer_data, denoised)
        == OsprayStatus::Ok);
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

  if (rendererType == "ao")
    renderer.setParam("aoSamples", 0);

  if (rendererType == "scivis") {
    renderer.setParam("shadows", true);
    renderer.setParam("aoSamples", 16);
  }

  renderer.setParam("backgroundColor", vec3f(1.0f));
  renderer.setParam("pixelSamples", samplesPerPixel);

  world = cpp::World();
}

void Base::SetLights()
{
  if (!lightsList.empty())
    world.setParam("light", cpp::CopiedData(lightsList));
}

void Base::RenderFrame()
{
  for (int frame = 0; frame < frames; ++frame)
    cpp::Future future = framebuffer.renderFrame(renderer, camera, world);
}

FromOsprayTesting::FromOsprayTesting()
{
  auto params = GetParam();

  sceneName = std::get<0>(params);
  rendererType = std::get<1>(params);
  samplesPerPixel = std::get<2>(params);
}

void FromOsprayTesting::SetUp()
{
  Base::SetUp();

  auto builder = ospray::testing::newBuilder(sceneName);
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);

  world = ospray::testing::buildWorld(builder);
  ospray::testing::release(builder);

  world.commit();

  auto worldBounds = world.getBounds<box3f>();

  ArcballCamera arcballCamera(worldBounds, imgSize);

  camera.setParam("position", arcballCamera.eyePos());
  camera.setParam("direction", arcballCamera.lookDir());
  camera.setParam("up", arcballCamera.upDir());

  if (rendererType == "pathtracer")
    renderer.setParam("limitIndirectLightSamples", false);
}

void FromOsprayTestingMaxDepth::SetUp()
{
  FromOsprayTesting::SetUp();

  // set up max depth texture
  {
    cpp::Texture maxDepthTex("texture2d");

    std::vector<float> maxDepth = {3.f, 3.f, 3.f, 3.f};
    maxDepthTex.setParam(
        "data", cpp::CopiedData(maxDepth.data(), vec2ul(2, 2)));
    maxDepthTex.setParam("format", OSP_TEXTURE_R32F);
    maxDepthTex.setParam("filter", OSP_TEXTURE_FILTER_NEAREST);
    maxDepthTex.commit();

    renderer.setParam("map_maxDepth", maxDepthTex);
  }
}

void FromOsprayTestingVariance::SetUp()
{
  FromOsprayTesting::SetUp();

  frames = 22;

  framebuffer = cpp::FrameBuffer(imgSize.x,
      imgSize.y,
      frameBufferFormat,
      OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_VARIANCE);

  renderer.setParam("varianceThreshold", 10.0f);
}

void FromOsprayTestingLightSamples::SetUp()
{
  FromOsprayTesting::SetUp();

  renderer.setParam("lightSamples", 8);
}

} // namespace OSPRayTestScenes
