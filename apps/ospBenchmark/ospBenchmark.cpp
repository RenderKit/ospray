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

// Test init/shutdown cylcle time //

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

struct RenderingObjects
{
  cpp::FrameBuffer f;
  cpp::Renderer r;
  cpp::Camera c;
  cpp::World w;
};

static RenderingObjects construct_test(std::string rendererType,
                                       std::string scene,
                                       vec2i imgSize = vec2i(1024, 768))
{
  // Frame buffer //

  cpp::FrameBuffer fb(
      imgSize, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_DEPTH);
  fb.resetAccumulation();

  // Scene //

  auto builder = testing::newBuilder(scene);
  testing::setParam(builder, "rendererType", rendererType);
  testing::commit(builder);

  auto world = testing::buildWorld(builder);
  testing::release(builder);

  world.commit();

  auto worldBounds = world.getBounds();

  // Camera //

  ArcballCamera arcballCamera(worldBounds, imgSize);

  cpp::Camera camera("perspective");
  camera.setParam("aspect", imgSize.x / (float)imgSize.y);
  camera.setParam("position", arcballCamera.eyePos());
  camera.setParam("direction", arcballCamera.lookDir());
  camera.setParam("up", arcballCamera.upDir());
  camera.commit();

  // Renderer //

  cpp::Renderer renderer(rendererType);
  renderer.commit();

  return {fb, renderer, camera, world};
}

void gravity_spheres_scivis(benchmark::State &state)
{
  auto objs = construct_test("scivis", "gravity_spheres_volume");

  for (auto _ : state) {
    objs.f.renderFrame(objs.r, objs.c, objs.w);
  }
}

void gravity_spheres_pathtracer(benchmark::State &state)
{
  auto objs = construct_test("pathtracer", "gravity_spheres_volume");

  for (auto _ : state) {
    objs.f.renderFrame(objs.r, objs.c, objs.w);
  }
}

BENCHMARK(gravity_spheres_scivis)->Unit(benchmark::kMillisecond);
BENCHMARK(gravity_spheres_pathtracer)->Unit(benchmark::kMillisecond);

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
