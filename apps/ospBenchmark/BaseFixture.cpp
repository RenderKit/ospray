// Copyright 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BaseFixture.h"
// ospcommon
#include "ospcommon/utility/SaveImage.h"

std::string BaseFixture::dumpFinalImageDir;

BaseFixture::BaseFixture(std::string r, std::string s)
    : rendererType(r), scene(s)
{}

void BaseFixture::SetUp(::benchmark::State &)
{
  framebuffer = cpp::FrameBuffer(
      imgSize, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_DEPTH);
  framebuffer.resetAccumulation();

  auto builder = testing::newBuilder(scene);
  testing::setParam(builder, "rendererType", rendererType);
  SetBuilderParameters(builder);
  testing::commit(builder);

  world = testing::buildWorld(builder);
  testing::release(builder);

  world.commit();

  auto worldBounds = world.getBounds();

  ArcballCamera arcballCamera(worldBounds, imgSize);

  camera = cpp::Camera("perspective");
  camera.setParam("aspect", imgSize.x / (float)imgSize.y);
  camera.setParam("position", arcballCamera.eyePos());
  camera.setParam("direction", arcballCamera.lookDir());
  camera.setParam("up", arcballCamera.upDir());
  camera.commit();

  renderer = cpp::Renderer(rendererType);
  SetRendererParameters(renderer);
  renderer.commit();
}

void BaseFixture::TearDown(::benchmark::State &)
{
  if (!dumpFinalImageDir.empty() && !outputFilename.empty()) {
    framebuffer.resetAccumulation();
    framebuffer.renderFrame(renderer, camera, world);

    auto *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);
    utility::writePPM(dumpFinalImageDir + "/" + outputFilename + ".ppm",
        imgSize.x,
        imgSize.y,
        fb);
    framebuffer.unmap(fb);
  }

  framebuffer = nullptr;
  renderer = nullptr;
  camera = nullptr;
  world = nullptr;
}
