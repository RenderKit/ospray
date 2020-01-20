// Copyright 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ArcballCamera.h"
// ospray_testing
#include "ospray_testing.h"
// google benchmark
#include "benchmark/benchmark.h"

using namespace ospray;
using namespace ospcommon;
using namespace ospcommon::math;

class BaseFixture : public ::benchmark::Fixture
{
 public:
  BaseFixture(std::string r, std::string s);

  void SetUp(::benchmark::State &) override;
  void TearDown(::benchmark::State &) override;

  virtual void SetBuilderParameters(testing::SceneBuilderHandle) {}
  virtual void SetRendererParameters(cpp::Renderer) {}

  static std::string dumpFinalImageDir;

 protected:
  std::string outputFilename;

  vec2i imgSize{1024, 768};
  std::string rendererType;
  std::string scene;

  cpp::FrameBuffer framebuffer{nullptr};
  cpp::Renderer renderer{nullptr};
  cpp::Camera camera{nullptr};
  cpp::World world{nullptr};
};

#define OSPRAY_DEFINE_BENCHMARK(FixtureName)                                   \
  BENCHMARK_DEFINE_F(FixtureName, )                                            \
  (benchmark::State & st)                                                      \
  {                                                                            \
    for (auto _ : st) {                                                        \
      framebuffer.renderFrame(renderer, camera, world);                        \
    }                                                                          \
  }                                                                            \
                                                                               \
  BENCHMARK_REGISTER_F(FixtureName, )->Unit(benchmark::kMillisecond);
