// ======================================================================== //
// Copyright 2016 Intel Corporation                                         //
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

#include <ospray_cpp/Camera.h>
#include <ospray_cpp/Model.h>
#include <ospray_cpp/Renderer.h>

namespace bench {
void writePPM(const std::string &fileName, const int sizeX, const int sizeY,
              const uint32_t *pixel);
}

struct OSPRayFixture {
  // Fixture hayai interface //

  void SetUp();
  void TearDown();

  // Fixture data //

  static std::unique_ptr<ospray::cpp::Renderer>    renderer;
  static std::unique_ptr<ospray::cpp::Camera>      camera;
  static std::unique_ptr<ospray::cpp::Model>       model;
  static std::unique_ptr<ospray::cpp::FrameBuffer> fb;

  // Command-line configuration data //

  static std::string imageOutputFile;
  static std::string scriptFile;

  static std::vector<std::string> benchmarkModelFiles;

  static int width;
  static int height;

  static size_t numBenchFrames;
  static size_t numWarmupFrames;
  static bool logFrameTimes;

  static ospcommon::vec3f bg_color;

  using Millis = std::chrono::duration<double, std::ratio<1, 1000>>;
  static std::unique_ptr<pico_bench::Benchmarker<Millis>> benchmarker;
};
