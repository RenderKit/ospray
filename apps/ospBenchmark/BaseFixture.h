// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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

#define OSPRAY_DEFINE_BENCHMARK(FixtureName)            \
  BENCHMARK_DEFINE_F(FixtureName, )                     \
  (benchmark::State & st)                               \
  {                                                     \
    for (auto _ : st) {                                 \
      framebuffer.renderFrame(renderer, camera, world); \
    }                                                   \
  }                                                     \
                                                        \
  BENCHMARK_REGISTER_F(FixtureName, )->Unit(benchmark::kMillisecond);
