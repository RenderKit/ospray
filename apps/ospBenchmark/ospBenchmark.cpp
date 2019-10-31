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

#include "ArcballCamera.h"
// ospray_testing
#include "ospray_testing.h"
// google benchmark
#include "benchmark/benchmark.h"

using namespace ospray;
using namespace ospcommon;
using namespace ospcommon::math;

// Test init/shutdown cycle time //////////////////////////////////////////////

static void ospInit_ospShutdown(benchmark::State &state)
{
  ospShutdown();

  for (auto _ : state) {
    ospInit();
    ospShutdown();
  }

  ospInit();
}

BENCHMARK(ospInit_ospShutdown)->Unit(benchmark::kMillisecond);

// Test rendering scenes from 'ospray_testing' ////////////////////////////////

class BaseBenchmark : public ::benchmark::Fixture
{
 public:
  BaseBenchmark(std::string r, std::string s) : rendererType(r), scene(s) {}

  void SetUp(::benchmark::State &) override
  {
    framebuffer = cpp::FrameBuffer(
        imgSize, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_DEPTH);
    framebuffer.resetAccumulation();

    auto builder = testing::newBuilder(scene);
    testing::setParam(builder, "rendererType", rendererType);
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
    renderer.commit();
  }

  void BenchmarkCase(::benchmark::State &state) override
  {
    for (auto _ : state) {
      framebuffer.renderFrame(renderer, camera, world);
    }
  }

 protected:
  vec2i imgSize{1024, 768};
  std::string rendererType;
  std::string scene;

  cpp::FrameBuffer framebuffer;
  cpp::Renderer renderer;
  cpp::Camera camera;
  cpp::World world;
};

class GravitySpheres : public BaseBenchmark
{
 public:
  GravitySpheres() : BaseBenchmark("pathtracer", "gravity_spheres_volume") {}
};

BENCHMARK_DEFINE_F(GravitySpheres, gravity_spheres_pathtracer)
(benchmark::State &st)
{
  for (auto _ : st) {
    framebuffer.renderFrame(renderer, camera, world);
  }
}

BENCHMARK_REGISTER_F(GravitySpheres, gravity_spheres_pathtracer)
    ->Unit(benchmark::kMillisecond);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  ospInit();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

  ospShutdown();

  return 0;
}
