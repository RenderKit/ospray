// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ArcballCamera.h"
// ospray_testing
#include "ospray_testing.h"
// google benchmark
#include "benchmark/benchmark.h"

using namespace ospray;
using namespace rkcommon;
using namespace rkcommon::math;

class BaseFixture : public ::benchmark::Fixture
{
 public:
  BaseFixture(const std::string &s, const std::string &r);

  void SetName(const std::string &n)
  {
    ::benchmark::Fixture::SetName(n.c_str());
    name = n;
  }

  void SetUp(::benchmark::State &) override;
  void TearDown(::benchmark::State &) override;

  virtual void SetBuilderParameters(testing::SceneBuilderHandle) {}
  virtual void SetRendererParameters(cpp::Renderer) {}

  static std::string dumpFinalImageDir;

 protected:
  std::string name;

  vec2i imgSize{1024, 768};
  std::string scene;
  std::string rendererType;

  cpp::FrameBuffer framebuffer{nullptr};
  cpp::Renderer renderer{nullptr};
  cpp::Camera camera{nullptr};
  cpp::World world{nullptr};
};

// Use line number to generate unique identifier
// Must not use the same FixtureName across different compilation units (.cpp
// files) or conflicts may appear
#define CAT2(a, b) a##b
#define CAT(a, b) CAT2(a, b)
#define UNIQUE_NAME(name) CAT(name, __LINE__)

#define OSPRAY_DEFINE_BENCHMARK(FixtureName, ...)                              \
  class UNIQUE_NAME(FixtureName) : public FixtureName                          \
  {                                                                            \
   public:                                                                     \
    UNIQUE_NAME(FixtureName)() : FixtureName(__VA_ARGS__) {}                   \
                                                                               \
   protected:                                                                  \
    virtual void BenchmarkCase(::benchmark::State &);                          \
  };                                                                           \
  void UNIQUE_NAME(FixtureName)::BenchmarkCase(benchmark::State &st)           \
  {                                                                            \
    for (auto _ : st) {                                                        \
      framebuffer.renderFrame(renderer, camera, world);                        \
    }                                                                          \
    st.SetItemsProcessed(st.iterations());                                     \
  }                                                                            \
  BENCHMARK_PRIVATE_REGISTER_F(UNIQUE_NAME(FixtureName))                       \
      ->Unit(benchmark::kMillisecond)                                          \
      ->UseRealTime();
