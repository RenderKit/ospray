// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BaseFixture.h"
#include "rkcommon/utility/SaveImage.h"

#include <algorithm>

std::string BaseFixture::dumpFinalImageDir;

BaseFixture::BaseFixture(
    const std::string &n, const std::string &s, const std::string &r)
    : name(n + "/" + r), scene(s), rendererType(r)
{
  ::benchmark::Fixture::SetName(name.c_str());
}

void BaseFixture::Init()
{
  framebuffer = cpp::FrameBuffer(imgSize.x,
      imgSize.y,
      OSP_FB_SRGBA,
      OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_DEPTH);
  framebuffer.resetAccumulation();

  auto builder = testing::newBuilder(scene);
  testing::setParam(builder, "rendererType", rendererType);
  SetBuilderParameters(builder);
  testing::commit(builder);

  world = testing::buildWorld(builder);
  testing::release(builder);

  world.commit();

  auto worldBounds = world.getBounds<box3f>();

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

void BaseFixture::Shutdown()
{
  framebuffer = nullptr;
  renderer = nullptr;
  camera = nullptr;
  world = nullptr;
}

void BaseFixture::TearDown(::benchmark::State &)
{
  if (!dumpFinalImageDir.empty() && !name.empty()) {
    std::string of = name;
    std::replace(of.begin(), of.end(), '/', '_');

    framebuffer.resetAccumulation();
    framebuffer.renderFrame(renderer, camera, world);
    auto *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);
    utility::writePPM(
        dumpFinalImageDir + "/" + of + ".ppm", imgSize.x, imgSize.y, fb);
    framebuffer.unmap(fb);
  }

  Shutdown();
}