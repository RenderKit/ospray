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

  samplesPerPixel = 2;
}

void IDBuffer::SetUp()
{
  Base::SetUp();

  framebuffer =
      cpp::FrameBuffer(imgSize.x, imgSize.y, frameBufferFormat, idBuffer);

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

class MiscBuffer
    : public Base,
      public ::testing::TestWithParam<std::tuple<OSPFrameBufferChannel,
          const char * /*camera*/,
          const char * /*renderer*/>>
{
 public:
  MiscBuffer();
  void SetUp() override;
  void PerformRenderTest() override;

 protected:
  OSPFrameBufferChannel channel{OSP_FB_DEPTH};
  bool projectedDepth{false};
  std::string cameraType{"perspective"};
};

MiscBuffer::MiscBuffer()
{
  auto params = GetParam();
  channel = std::get<0>(params);
  projectedDepth = channel == -1;
  if (projectedDepth)
    channel = OSP_FB_DEPTH;
  cameraType = std::get<1>(params);
  rendererType = std::get<2>(params);
  frameBufferFormat = OSP_FB_SRGBA;
  // check accumulation (in particular depth as minimum)
  samplesPerPixel = 4;
  frames = 4;
}

void MiscBuffer::SetUp()
{
  Base::SetUp();

  auto builder = ospray::testing::newBuilder("test_pt_principled_glass");
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);

  world = ospray::testing::buildWorld(builder);
  ospray::testing::release(builder);

  // FIXME color format OSP_FB_NONE should be sufficient, but MPI's DFB
  // makes too many assumptions, like a FB with OSP_FB_NONE won't map
  // *any* channel
  framebuffer = cpp::FrameBuffer(
      imgSize.x, imgSize.y, OSP_FB_RGBA8, OSP_FB_ACCUM | channel);
  framebuffer.setParam("projectedDepth", projectedDepth);
  framebuffer.setParam("targetFrames", frames);
}

void MiscBuffer::PerformRenderTest()
{
  world.commit();
  camera = cpp::Camera(cameraType);
  camera.setParam("position", vec3f(0, 0, -1.5));
  // zoom into interesting area
  if (cameraType == "perspective") {
    camera.setParam("fovy", 106.0f);
    camera.setParam("aspect", imgSize.x / (float)imgSize.y);
  } else {
    camera.setParam("imageStart", vec2f(0.05, 0.2));
    camera.setParam("imageEnd", vec2f(0.45, 0.8));
    // look to side to also get negative projected depth
    camera.setParam("direction", vec3f(-1, 0, 0));
  }
  camera.commit();
  renderer.commit();

  framebuffer.commit();
  framebuffer.resetAccumulation();

  RenderFrame();

  auto *framebuffer_data = (float *)framebuffer.map(channel);
  std::vector<uint32_t> framebufferData(imgSize.product());
  for (int i = 0; i < imgSize.product(); i++) {
    vec3f col{0.f};
    if (!std::isinf(framebuffer_data[i])) {
      if (channel == OSP_FB_DEPTH) {
        // map to range and "convert" to srgb
        const float d = abs(framebuffer_data[i]) - 1.7f;
        if (d > 0.0f)
          col = vec3f(sqrt(0.28f * d));
        if (framebuffer_data[i] < 0) // negative depth in red
          col.y = col.z = 0.0f;
      } else
        col = (channel == OSP_FB_POSITION ? 0.3f : 1.0f)
            * abs(vec3f(framebuffer_data[i * 3],
                framebuffer_data[i * 3 + 1],
                framebuffer_data[i * 3 + 2]));
    }
    framebufferData[i] = to_rgba8(col);
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

TEST_P(MiscBuffer, MiscBuffer)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Depth,
    MiscBuffer,
    ::testing::Combine(::testing::Values(OSP_FB_DEPTH, -1 /*projectedDepth*/),
        ::testing::Values("perspective", "panoramic"),
        ::testing::Values("scivis", "pathtracer")));

INSTANTIATE_TEST_SUITE_P(AOV,
    MiscBuffer,
    ::testing::Combine(
        ::testing::Values(
            OSP_FB_ALBEDO, OSP_FB_NORMAL, OSP_FB_FIRST_NORMAL, OSP_FB_POSITION),
        ::testing::Values("perspective"),
        ::testing::Values("scivis", "pathtracer")));

} // namespace OSPRayTestScenes
