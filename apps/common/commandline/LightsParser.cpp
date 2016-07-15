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

#include "LightsParser.h"

#include <ospray_cpp/Data.h>

#include <vector>

#include <common/miniSG/miniSG.h>

using namespace ospcommon;

DefaultLightsParser::DefaultLightsParser(ospray::cpp::Renderer renderer) :
  m_renderer(renderer),
  m_defaultDirLight_direction(.3, -1, -.2)
{
}

bool DefaultLightsParser::parse(int ac, const char **&av)
{
  std::vector<OSPLight> lights;

  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--sun-dir") {
      if (!strcmp(av[i+1],"none")) {
        ++i;
        m_defaultDirLight_direction = vec3f(0.f);
      } else {
        m_defaultDirLight_direction.x = atof(av[++i]);
        m_defaultDirLight_direction.y = atof(av[++i]);
        m_defaultDirLight_direction.z = atof(av[++i]);
      }
    } else if (arg == "--hdri-light") {
      // Syntax for HDRI light is the same as Embree:
        // --hdri-light <intensity> <image file>.(pfm|ppm)
        auto ospHdri = m_renderer.newLight("hdri");
        ospHdri.set("name", "hdri light");
        if (1) {//TODO: add commandline option for up direction.
          ospHdri.set("up", 0.f, 1.f, 0.f);
          ospHdri.set("dir", 1.f, 0.f, 0.0f);
        } else {// up = z
          ospHdri.set("up", 0.f, 0.f, 1.f);
          ospHdri.set("dir", 0.f, 1.f, 0.0f);
        }
        ospHdri.set( "intensity", atof(av[++i]));
        FileName imageFile(av[++i]);
        ospray::miniSG::Texture2D *lightMap = ospray::miniSG::loadTexture(imageFile.path(), imageFile.base());
        if (lightMap == NULL){
          std::cout << "Failed to load hdri-light texture '" << imageFile << "'" << std::endl;
        } else {
          std::cout << "Successfully loaded hdri-light texture '" << imageFile << "'" << std::endl;
        }
        OSPTexture2D ospLightMap = ospray::miniSG::createTexture2D(lightMap);
        ospHdri.set( "map", ospLightMap);
        ospHdri.commit();
        lights.push_back(ospHdri.handle());
    } else if (arg == "--backplate") {
        FileName imageFile(av[++i]);
        ospray::miniSG::Texture2D *backplate = ospray::miniSG::loadTexture(imageFile.path(), imageFile.base());
        if (backplate == NULL){
          std::cout << "Failed to load backplate texture '" << imageFile << "'" << std::endl;
        }
        OSPTexture2D ospBackplate = ospray::miniSG::createTexture2D(backplate);
        m_renderer.set("backplate", ospBackplate);   
    }
  }

  //TODO: Need to figure out where we're going to read lighting data from
  
  if (m_defaultDirLight_direction != vec3f(0.f)) {
    auto ospLight = m_renderer.newLight("DirectionalLight");
    if (ospLight.handle() == nullptr) {
      throw std::runtime_error("Failed to create a 'DirectionalLight'!");
    }
    ospLight.set("name", "sun");
    ospLight.set("color", 1.f, 1.f, 1.f);
    ospLight.set("direction", m_defaultDirLight_direction);
    ospLight.set("angularDiameter", 0.53f);
    ospLight.commit();
    lights.push_back(ospLight.handle());
  }

  auto lightArray = ospray::cpp::Data(lights.size(), OSP_OBJECT, lights.data());
  //lightArray.commit();
  m_renderer.set("lights", lightArray);

  finalize();

  return true;
}

void DefaultLightsParser::finalize()
{
  
}
