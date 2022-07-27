// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ArcballCamera.h"
#include "ospray_testing.h"
#include "rkcommon/utility/random.h"
#include "test_fixture.h"

extern OSPRayEnvironment *ospEnv;

namespace OSPRayTestScenes {

class IDBuffer
    : public Base,
      public ::testing::TestWithParam<std::tuple<OSPFrameBufferChannel,
          bool /*appIDs*/,
          const char * /*renderer*/>>
{
 public:
  IDBuffer();
  void SetUp() override;
  void PerformRenderTest() override;

 protected:
  OSPFrameBufferChannel idBuffer{OSP_FB_ID_PRIMITIVE};
  bool appIDs{false};
};

IDBuffer::IDBuffer()
{
  auto params = GetParam();
  idBuffer = std::get<0>(params);
  appIDs = std::get<1>(params);
  rendererType = std::get<2>(params);

  framebuffer =
      cpp::FrameBuffer(imgSize.x, imgSize.y, frameBufferFormat, idBuffer);
  samplesPerPixel = 2;
}

void IDBuffer::SetUp()
{
  Base::SetUp();

  instances.clear();

  auto builder = ospray::testing::newBuilder(
      idBuffer == OSP_FB_ID_PRIMITIVE ? "perlin_noise_volumes" : "instancing");
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::setParam(builder, "numInstances", vec2ui(10, 3));
  ospray::testing::setParam(builder, "useIDs", appIDs);
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

void IDBuffer::PerformRenderTest()
{
  if (!instances.empty())
    world.setParam("instance", cpp::CopiedData(instances));

  world.commit();

  ArcballCamera arcballCamera(world.getBounds<box3f>(), imgSize);
  camera.setParam("position", arcballCamera.eyePos());
  camera.setParam("direction", arcballCamera.lookDir());
  camera.setParam("up", arcballCamera.upDir());
  camera.setParam("fovy", 45.0f);
  camera.commit();

  renderer.commit();

  framebuffer.resetAccumulation();

  RenderFrame();

  auto *framebuffer_data = (uint32_t *)framebuffer.map(idBuffer);
  std::vector<uint32_t> framebufferData(imgSize.product());
  for (int i = 0; i < imgSize.product(); i++)
    framebufferData[i] = framebuffer_data[i] == -1u
        ? 0
        : to_rgba8(rkcommon::utility::makeRandomColor(framebuffer_data[i]));
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

TEST_P(IDBuffer, IDBuffer)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Primitive,
    IDBuffer,
    ::testing::Combine(::testing::Values(OSP_FB_ID_PRIMITIVE),
        ::testing::Values(false),
        ::testing::Values("scivis", "pathtracer")));

INSTANTIATE_TEST_SUITE_P(ObjectInstance,
    IDBuffer,
    ::testing::Combine(::testing::Values(OSP_FB_ID_OBJECT, OSP_FB_ID_INSTANCE),
        ::testing::Bool(),
        ::testing::Values("scivis", "pathtracer")));

} // namespace OSPRayTestScenes
