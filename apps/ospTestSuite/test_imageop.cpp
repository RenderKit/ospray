// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ospray_testing.h"
#include "test_fixture.h"

namespace OSPRayTestScenes {

class ImageOpBase : public Base
{
 public:
  ImageOpBase()
  {
    samplesPerPixel = 4;
  }
  void SetUp() override;

 protected:
  std::string imageOp;
};

void ImageOpBase::SetUp()
{
  Base::SetUp();

  instances.clear();

  auto builder = ospray::testing::newBuilder("cornell_box");
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);

  world = ospray::testing::buildWorld(builder);
  ospray::testing::release(builder);

  camera.setParam("position", vec3f(0.f, 0.f, -2.f));

  cpp::ImageOperation imgOp(imageOp);
  framebuffer.setParam("imageOperation", cpp::CopiedData(imgOp));
  framebuffer.commit();
}

#ifdef OSPRAY_MODULE_DENOISER
class DenoiserOp : public ImageOpBase, public ::testing::Test
{
 public:
  DenoiserOp()
  {
    ospLoadModule("denoiser");

    frameBufferFormat = OSP_FB_RGBA32F;
    rendererType = "pathtracer";
    imageOp = "denoiser";
  }
  void SetUp() override
  {
    ImageOpBase::SetUp();
  }
};
#endif

class ImageOp
    : public ImageOpBase,
      public ::testing::TestWithParam<
          std::tuple<const char * /*image op*/, const char * /*renderer*/>>
{
 public:
  ImageOp()
  {
    auto params = GetParam();
    imageOp = std::get<0>(params);
    rendererType = std::get<1>(params);
  }
  void SetUp() override
  {
    ImageOpBase::SetUp();
  }
};

// Test Instantiations //////////////////////////////////////////////////////

#ifdef OSPRAY_MODULE_DENOISER
TEST_F(DenoiserOp, DenoiserOp)
{
  PerformRenderTest();
}
#endif

TEST_P(ImageOp, ImageOp)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(
    DebugOp, ImageOp, ::testing::Values(std::make_tuple("debug", "scivis")));

} // namespace OSPRayTestScenes
