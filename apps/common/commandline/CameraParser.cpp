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

#include "CameraParser.h"

bool DefaultCameraParser::parse(int ac, const char **&av)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--camera" || arg == "-c") {
      m_cameraType = av[++i];
    } else if (arg == "-vp" || arg == "--eye") {
      auto &pos = m_eye;
      pos.x = atof(av[++i]);
      pos.y = atof(av[++i]);
      pos.z = atof(av[++i]);
    } else if (arg == "-vu" || arg == "--up") {
      auto &up = m_up;
      up.x = atof(av[++i]);
      up.y = atof(av[++i]);
      up.z = atof(av[++i]);
    } else if (arg == "-vi" || arg == "--gaze") {
      auto &at = m_gaze;
      at.x = atof(av[++i]);
      at.y = atof(av[++i]);
      at.z = atof(av[++i]);
    }
  }

  finalize();

  return true;
}

ospray::cpp::Camera DefaultCameraParser::camera()
{
  return m_camera;
}

void DefaultCameraParser::finalize()
{
  if (m_cameraType.empty())
    m_cameraType = "perspective";

  m_camera = ospray::cpp::Camera(m_cameraType.c_str());
  m_camera.set("pos", m_eye);
  m_camera.set("up",  m_up);
  m_camera.set("dir", m_gaze - m_eye);
  m_camera.commit();
}
