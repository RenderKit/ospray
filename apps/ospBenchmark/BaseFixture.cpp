// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BaseFixture.h"
#include "rkcommon/utility/SaveImage.h"

#include <algorithm>

std::string BaseFixture::dumpFinalImageDir;

BaseFixture::BaseFixture(const std::string &s, const std::string &r)
    : scene(s), rendererType(r)
{
  SetName(s + "/" + r);
}

void BaseFixture::SetUp(::benchmark::State &)
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

  framebuffer = nullptr;
  renderer = nullptr;
  camera = nullptr;
  world = nullptr;
}

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "boxes", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "random_spheres", "ao");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "random_spheres", "scivis");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "random_spheres", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "streamlines", "ao");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "streamlines", "scivis");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "streamlines", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "planes", "ao");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "planes", "scivis");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "planes", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "clip_gravity_spheres_volume", "ao");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "clip_gravity_spheres_volume", "scivis");
OSPRAY_DEFINE_BENCHMARK(
    BaseFixture, "clip_gravity_spheres_volume", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "clip_perlin_noise_volumes", "ao");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "clip_perlin_noise_volumes", "scivis");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "clip_perlin_noise_volumes", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "clip_particle_volume", "ao");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "clip_particle_volume", "scivis");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "clip_particle_volume", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "particle_volume", "ao");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "particle_volume", "scivis");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "particle_volume", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "particle_volume_isosurface", "ao");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "particle_volume_isosurface", "scivis");
OSPRAY_DEFINE_BENCHMARK(
    BaseFixture, "particle_volume_isosurface", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "vdb_volume", "ao");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "vdb_volume", "scivis");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "vdb_volume", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "unstructured_volume", "ao");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "unstructured_volume", "scivis");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "unstructured_volume", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "unstructured_volume_isosurface", "ao");
OSPRAY_DEFINE_BENCHMARK(
    BaseFixture, "unstructured_volume_isosurface", "scivis");
OSPRAY_DEFINE_BENCHMARK(
    BaseFixture, "unstructured_volume_isosurface", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(BaseFixture, "gravity_spheres_amr", "ao");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "gravity_spheres_amr", "scivis");
OSPRAY_DEFINE_BENCHMARK(BaseFixture, "gravity_spheres_amr", "pathtracer");
