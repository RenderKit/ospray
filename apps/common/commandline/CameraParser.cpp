// ======================================================================== //
// Copyright 2016 SURVICE Engineering Company                               //
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

#include <stdexcept>
#include <fstream>
#include <string>

#include "CameraParser.h"


bool DefaultCameraParser::parse(int ac, const char **&av)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--camera" || arg == "-c") {
      m_cameraType = av[++i];
    } else if (arg == "-v" || arg == "--view") {
      std::ifstream fin(av[++i]);
      if (!fin.is_open())
      {
        throw std::runtime_error("Failed to open \"" + std::string(av[i]) +
                                 "\" for reading");
      }

      auto token = std::string("");
      while (fin >> token)
      {
        if (token == "-vp")
          fin >> m_eye.x >> m_eye.y >> m_eye.z;
        else if (token == "-vu")
          fin >> m_up.x >> m_up.y >> m_up.z;
        else if (token == "-vi")
          fin >> m_gaze.x >> m_gaze.y >> m_gaze.z;
        else if (token == "-fv")
          fin >> m_fovy;
        else
          throw std::runtime_error("Unrecognized token:  \"" + token + '\"');
      }

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
    } else if (arg == "-fv" || arg == "--fovy") {
      m_fovy = atof(av[++i]);
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
  m_camera.set("pos",  m_eye);
  m_camera.set("up",   m_up);
  m_camera.set("dir",  m_gaze - m_eye);
  m_camera.set("fovy", m_fovy);
  m_camera.commit();
}
