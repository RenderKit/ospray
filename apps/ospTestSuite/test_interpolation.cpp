// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ArcballCamera.h"
#include "ospray_testing.h"
#include "rkcommon/utility/random.h"
#include "test_fixture.h"

extern OSPRayEnvironment *ospEnv;

namespace OSPRayTestScenes {

class Interpolation
    : public Base,
      public ::testing::TestWithParam<std::tuple<bool /*use subd*/,
          unsigned int /*attribute*/,
          unsigned int /*interpolation*/>>
{
 public:
  Interpolation();
  void SetUp() override;
  void PerformRenderTest() override;

 protected:
  OSPFrameBufferChannel idBuffer{OSP_FB_COLOR};
  bool useSubd{false};
  unsigned int attribute{0};
  unsigned int interpolation{0};
};

Interpolation::Interpolation()
{
  samplesPerPixel = 2;
  rendererType = "scivis";

  auto params = GetParam();
  useSubd = std::get<0>(params);
  attribute = std::get<1>(params);
  interpolation = std::get<2>(params);

  if (attribute == 2)
    idBuffer = OSP_FB_NORMAL;

  framebuffer =
      cpp::FrameBuffer(imgSize.x, imgSize.y, frameBufferFormat, idBuffer);
}

void Interpolation::SetUp()
{
  Base::SetUp();

  instances.clear();

  auto builder = ospray::testing::newBuilder("interpolation");
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::setParam(builder, "useSubd", useSubd);
  ospray::testing::setParam(builder, "attribute", attribute);
  ospray::testing::setParam(builder, "interpolation", interpolation);
  ospray::testing::commit(builder);

  world = ospray::testing::buildWorld(builder);
  ospray::testing::release(builder);
}

inline uint8_t to_uc(const float f)
{
  return 255.f * clamp(f, 0.f, 1.0f);
}
inline uint32_t to_rgba8(const vec3f c)
{
  return to_uc(c.x) | (to_uc(c.y) << 8) | (to_uc(c.z) << 16) | 0xff000000;
}

void Interpolation::PerformRenderTest()
{
  if (!instances.empty())
    world.setParam("instance", cpp::CopiedData(instances));

  world.commit();

  ArcballCamera arcballCamera(world.getBounds<box3f>(), imgSize);
  camera.setParam("position", arcballCamera.eyePos());
  camera.setParam("direction", arcballCamera.lookDir());
  camera.setParam("up", arcballCamera.upDir());
  camera.commit();

  renderer.commit();

  framebuffer.resetAccumulation();

  RenderFrame();

  auto *framebuffer_data = (uint32_t *)framebuffer.map(idBuffer);
  std::vector<uint32_t> framebufferData(imgSize.product());
  for (int i = 0; i < imgSize.product(); i++) {
    if (attribute == 2) { // normals
      const vec3f normal = ((vec3f *)framebuffer_data)[i];
      framebufferData[i] = to_rgba8(0.5f * normal + 0.5f);
    } else
      framebufferData[i] = framebuffer_data[i];
  }
  framebuffer.unmap(framebuffer_data);

  if (ospEnv->GetDumpImg()) {
    EXPECT_EQ(
        imageTool->saveTestImage(framebufferData.data()), OsprayStatus::Ok);
  } else {
    EXPECT_EQ(imageTool->compareImgWithBaseline(framebufferData.data()),
        OsprayStatus::Ok);
  }
}

// Test Instantiations //////////////////////////////////////////////////////

TEST_P(Interpolation, Interpolation)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Color,
    Interpolation,
    ::testing::Combine(::testing::Bool(),
        ::testing::Values(0),
        ::testing::Values(0, 1, 2, 3)));

INSTANTIATE_TEST_SUITE_P(Texcoord,
    Interpolation,
    ::testing::Combine(
        ::testing::Bool(), ::testing::Values(1), ::testing::Values(0, 1)));

INSTANTIATE_TEST_SUITE_P(Normal,
    Interpolation,
    ::testing::Combine(::testing::Values(false),
        ::testing::Values(2),
        ::testing::Values(0, 1)));

} // namespace OSPRayTestScenes
