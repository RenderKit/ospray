// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include <common/commandline/CommandLineParser.h>
#include <ospray_cpp/Camera.h>

#include <string>

class CameraParser : public CommandLineParser
{
public:
  virtual ospray::cpp::Camera camera() = 0;
};

class DefaultCameraParser : public CameraParser
{
public:
  DefaultCameraParser() = default;
  bool parse(int ac, const char **&av) override;
  ospray::cpp::Camera camera() override;

protected:

  std::string         m_cameraType;
  ospray::cpp::Camera m_camera;

  ospcommon::vec3f m_eye {-1,  1, -1};
  ospcommon::vec3f m_up  { 1, -1,  1};
  ospcommon::vec3f m_gaze{ 0,  1,  0};

private:

  void finalize();
};
