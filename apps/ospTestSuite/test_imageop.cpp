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
  std::string imageOpType;
  cpp::ImageOperation imageOp;
};

void ImageOpBase::SetUp()
{
  Base::SetUp();

  instances.clear();

  auto builder = ospray::testing::newBuilder("random_discs");
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);

  world = ospray::testing::buildWorld(builder);
  ospray::testing::release(builder);

  camera.setParam("position", vec3f(0.f, 0.f, -2.5f));

  imageOp = cpp::ImageOperation(imageOpType);

  frames = 1;
  framebuffer = cpp::FrameBuffer(imgSize.x,
      imgSize.y,
      frameBufferFormat,
      OSP_FB_COLOR | OSP_FB_ALBEDO | OSP_FB_NORMAL);
  framebuffer.setParam("imageOperation", cpp::CopiedData(imageOp));
  framebuffer.commit();
}

#ifdef OSPRAY_MODULE_DENOISER
class DenoiserOp : public ImageOpBase,
                   public ::testing::TestWithParam<
                       std::tuple<bool /*denoiseAlpha*/, OSPDenoiserQuality>>
{
 public:
  DenoiserOp()
  {
    ospLoadModule("denoiser");

    rendererType = "pathtracer";
    imageOpType = "denoiser";
  }
  void SetUp() override
  {
    ImageOpBase::SetUp();
    auto params = GetParam();
    imageOp.setParam("denoiseAlpha", std::get<0>(params));
    imageOp.setParam("quality", std::get<1>(params));
    imageOp.commit();
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
    imageOpType = std::get<0>(params);
    rendererType = std::get<1>(params);
  }
  void SetUp() override
  {
    ImageOpBase::SetUp();
  }
};

// Test Instantiations //////////////////////////////////////////////////////

#ifdef OSPRAY_MODULE_DENOISER
TEST_P(DenoiserOp, DenoiserOp)
{
  PerformRenderTest();
}
INSTANTIATE_TEST_SUITE_P(DenoiserOp,
    DenoiserOp,
    ::testing::Combine(::testing::Bool(),
        ::testing::Values(OSP_DENOISER_QUALITY_LOW,
            OSP_DENOISER_QUALITY_MEDIUM,
            OSP_DENOISER_QUALITY_HIGH)));
#endif

TEST_P(ImageOp, ImageOp)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(
    DebugOp, ImageOp, ::testing::Values(std::make_tuple("debug", "scivis")));

} // namespace OSPRayTestScenes
