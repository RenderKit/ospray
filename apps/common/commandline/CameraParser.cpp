// ======================================================================== //
// Copyright 2016 SURVICE Engineering Company                               //
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

#include <stdexcept>
#include <fstream>
#include <string>

#include "CameraParser.h"


bool DefaultCameraParser::parse(int ac, const char **&av)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--camera" || arg == "-c") {
      cameraType = av[++i];
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
          fin >> eye.x >> eye.y >> eye.z;
        else if (token == "-vu")
          fin >> up.x >> up.y >> up.z;
        else if (token == "-vi")
          fin >> gaze.x >> gaze.y >> gaze.z;
        else if (token == "-fv")
          fin >> fovy;
        else
          throw std::runtime_error("Unrecognized token:  \"" + token + '\"');
      }

    } else if (arg == "-vp" || arg == "--eye") {
      eye.x = atof(av[++i]);
      eye.y = atof(av[++i]);
      eye.z = atof(av[++i]);
    } else if (arg == "-vu" || arg == "--up") {
      up.x = atof(av[++i]);
      up.y = atof(av[++i]);
      up.z = atof(av[++i]);
    } else if (arg == "-vi" || arg == "--gaze") {
      gaze.x = atof(av[++i]);
      gaze.y = atof(av[++i]);
      gaze.z = atof(av[++i]);
    } else if (arg == "-fv" || arg == "--fovy") {
      fovy = atof(av[++i]);
    }
  }

  finalize();

  return true;
}

ospray::cpp::Camera DefaultCameraParser::camera()
{
  return parsedCamera;
}

void DefaultCameraParser::finalize()
{
  if (cameraType.empty())
    cameraType = "perspective";

  parsedCamera = ospray::cpp::Camera(cameraType.c_str());
  parsedCamera.set("pos",  eye);
  parsedCamera.set("up",   up);
  parsedCamera.set("dir",  gaze - eye);
  parsedCamera.set("fovy", fovy);
  parsedCamera.commit();
}
