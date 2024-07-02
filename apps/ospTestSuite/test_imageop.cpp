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
  framebuffer.setParam("imageOperation", cpp::CopiedData(imageOp));
  framebuffer.commit();
}

#ifdef OSPRAY_MODULE_DENOISER
class DenoiserOp : public ImageOpBase,
                   public ::testing::TestWithParam<bool /*denoiseAlpha*/>
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
    imageOp.setParam("denoiseAlpha", GetParam());
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
INSTANTIATE_TEST_SUITE_P(DenoiserOp, DenoiserOp, ::testing::Bool());
#endif

TEST_P(ImageOp, ImageOp)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(
    DebugOp, ImageOp, ::testing::Values(std::make_tuple("debug", "scivis")));

} // namespace OSPRayTestScenes
