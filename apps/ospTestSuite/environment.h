// ======================================================================== //
// Copyright 2017-2019 Intel Corporation                                    //
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

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ospray/ospray_cpp.h"
using namespace ospray;

#include "ospcommon/math/vec.h"
using namespace ospcommon::math;

class OSPRayEnvironment : public ::testing::Environment
{
 private:
  bool dumpImg;
  std::string rendererType;
  std::string deviceType;
  std::string baselineDir;
  std::string failedDir;
  vec2i imgSize {1024, 768};
  OSPDevice device {nullptr};

 public:
  OSPRayEnvironment(int argc, char **argv);
  ~OSPRayEnvironment() = default;

  bool GetDumpImg() const
  {
    return dumpImg;
  }
  std::string GetRendererType() const
  {
    return rendererType;
  }
  vec2i GetImgSize() const
  {
    return imgSize;
  }
  std::string GetDeviceType() const
  {
    return deviceType;
  }
  std::string GetBaselineDir() const
  {
    return baselineDir;
  }
  std::string GetFailedDir() const
  {
    return failedDir;
  }

  void ParsArgs(int argc, char **argv);
  std::string GetStrArgValue(std::string *arg) const;
  int GetNumArgValue(std::string *arg) const;
};
