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


#include "hayai/hayai.hpp"

#include <ospray_cpp/Camera.h>
#include <ospray_cpp/Model.h>
#include <ospray_cpp/Renderer.h>

struct OSPRayFixture : public hayai::Fixture
{
  // Fixture hayai interface //

  void SetUp() override;
  void TearDown() override;

  // Fixture data //

  static std::unique_ptr<ospray::cpp::Renderer>    renderer;
  static std::unique_ptr<ospray::cpp::Camera>      camera;
  static std::unique_ptr<ospray::cpp::Model>       model;
  static std::unique_ptr<ospray::cpp::FrameBuffer> fb;

  // Command-line configuration data //

  static std::string imageOutputFile;

  static std::vector<std::string> benchmarkModelFiles;

  static int width;
  static int height;

  static int numWarmupFrames;

  static ospcommon::vec3f bg_color;
};
