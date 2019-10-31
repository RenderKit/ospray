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

#include "BaseFixture.h"

BaseFixture::BaseFixture(std::string r, std::string s)
    : rendererType(r), scene(s)
{
}

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
  framebuffer = nullptr;
  renderer    = nullptr;
  camera      = nullptr;
  world       = nullptr;
}

#if 1
void BaseFixture::BenchmarkCase(::benchmark::State &state)
{
  for (auto _ : state) {
    framebuffer.renderFrame(renderer, camera, world);
  }
}
#endif
