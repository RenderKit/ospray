// ======================================================================== //
// Copyright 2017 Intel Corporation                                         //
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

#include <chrono>
#include "pico_bench/pico_bench.h"

#include <ospray/ospray_cpp/Camera.h>
#include <ospray/ospray_cpp/Model.h>
#include <ospray/ospray_cpp/Renderer.h>

#include <deque>

namespace bench {
void writePPM(const std::string &fileName, const int sizeX, const int sizeY,
              const uint32_t *pixel);
}

struct OSPRayFixture {
  using Millis = std::chrono::duration<double, std::ratio<1, 1000>>;

  // Create a benchmarker with default image dims and bench frame count,
  // image dims = 1024x1024
  // warm up frames = 10
  // benchmark frames = 100
  OSPRayFixture(ospray::cpp::Renderer renderer, ospray::cpp::Camera camera,
                 ospray::cpp::Model model);
  // Benchmark the scene, passing no params (or 0 for warmUpFrames or benchFrames) will
  // use the default configuration stored in the fixture, e.g. what was parsed from
  // the command line.
  pico_bench::Statistics<Millis> benchmark(const size_t warmUpFrames = 0, const size_t benchFrames = 0);
  // Save the rendered image. The name should not be suffixed by .ppm, it will be appended
  // for you.
  void saveImage(const std::string &fname);
  // Change the framebuffer dimensions for the benchmark. If either is 0, the previously
  // set width or height will be used accordingly. Can also change the framebuffer flags used
  // to enable/disable accumulation.
  void setFrameBuffer(const int w = 0, const int h = 0, const int fbFlags = OSP_FB_COLOR | OSP_FB_ACCUM);

  ospray::cpp::Renderer renderer;
  ospray::cpp::Camera camera;
  ospray::cpp::Model model;

  size_t defaultWarmupFrames;
  size_t defaultBenchFrames;

private:
  ospray::cpp::FrameBuffer fb;

  // Configuration data //

  int width;
  int height;
  int framebufferFlags;
};

