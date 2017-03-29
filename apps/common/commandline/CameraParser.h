// ======================================================================== //
// Copyright 2016 SURVICE Engineering Company                               //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include <common/commandline/CommandLineExport.h>
#include <common/commandline/CommandLineParser.h>
#include <ospray/ospray_cpp/Camera.h>

#include <string>

class OSPRAY_COMMANDLINE_INTERFACE CameraParser : public CommandLineParser
{
public:
  virtual ~CameraParser() = default;
  virtual ospray::cpp::Camera camera() = 0;
};

class OSPRAY_COMMANDLINE_INTERFACE DefaultCameraParser : public CameraParser
{
public:
  DefaultCameraParser() = default;
  bool parse(int ac, const char **&av) override;
  ospray::cpp::Camera camera() override;

protected:

  std::string         cameraType;
  ospray::cpp::Camera parsedCamera;

  ospcommon::vec3f eye {.5,  .5, 1};
  ospcommon::vec3f up  { 0, 1,  0};
  ospcommon::vec3f gaze{ 0,  -.1, -1};
  float fovy{ 60.f };

private:

  void finalize();
};
