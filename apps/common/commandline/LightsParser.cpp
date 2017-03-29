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

#include "LightsParser.h"

#include <ospray/ospray_cpp/Data.h>

#include <vector>

#include <common/miniSG/miniSG.h>

using namespace ospcommon;

DefaultLightsParser::DefaultLightsParser(ospray::cpp::Renderer renderer) :
  renderer(renderer),
  defaultDirLight_direction(.3, -1, -.2), defaultDirLight_intensity(3.14f)
{
}

bool DefaultLightsParser::parse(int ac, const char **&av)
{
  std::vector<OSPLight> lights;
  
  int HDRI_up = 1;//y
  float HDRI_intensity = 0.f;
  const char * HDRI_file_name;
  vec4f ambient(.85,.9,1,.2*3.14);

  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--sun-dir") {
      if (!strcmp(av[i+1],"none")) {
        ++i;
        defaultDirLight_direction = vec3f(0.f);
      } else {
        defaultDirLight_direction.x = atof(av[++i]);
        defaultDirLight_direction.y = atof(av[++i]);
        defaultDirLight_direction.z = atof(av[++i]);
      }
    } else if (arg == "--sun-int") {
        defaultDirLight_intensity = atof(av[++i]);
    } else if (arg == "--ambient") {
        ambient.x = atof(av[++i]);
        ambient.y = atof(av[++i]);
        ambient.z = atof(av[++i]);
        ambient.w = atof(av[++i]);
    } else if (arg == "--hdri-light") {
        if (i+2 >= ac)
          throw std::runtime_error("Not enough arguments! Usage:\n\t"
              "--hdri-light <intensity> <image file>.(pfm|ppm)");
      
        HDRI_intensity = atof(av[++i]);
        HDRI_file_name = av[++i];
    } else if (arg == "--backplate") {
        FileName imageFile(av[++i]);
        ospray::miniSG::Texture2D *backplate = ospray::miniSG::loadTexture(imageFile.path(), imageFile.base());
        if (backplate == NULL){
          std::cout << "Failed to load backplate texture '" << imageFile << "'" << std::endl;
        }
        OSPTexture2D ospBackplate = ospray::miniSG::createTexture2D(backplate);
        renderer.set("backplate", ospBackplate);   
    } else if (arg == "--hdri-up") {
      if (!strcmp(av[i+1],"x") || !strcmp(av[i+1],"X")) {
        HDRI_up = 0;
      } else if (!strcmp(av[i+1],"y") || !strcmp(av[i+1],"Y")) {
        HDRI_up = 1;
      } else if (!strcmp(av[i+1],"z") || !strcmp(av[i+1],"Z")) {
        HDRI_up = 2;
      } else {
        printf("--hdri-up must be x, y, or z.\n");
      }
    }
  }// Done reading commandline args.
  
  // HDRI environment light.
  if (HDRI_intensity > 0.f) {
    auto ospHdri = renderer.newLight("hdri");
    ospHdri.set("name", "hdri light");
    if (HDRI_up == 0) {// up = x
      ospHdri.set("up", 1.f, 0.f, 0.f);
      ospHdri.set("dir", 0.f, 0.f, 1.0f);
    } else if (HDRI_up == 1) {// up = y
      ospHdri.set("up", 0.f, 1.f, 0.f);
      ospHdri.set("dir", 1.f, 0.f, 0.0f);
    } else if (HDRI_up == 2) {// up = z
      ospHdri.set("up", 0.f, 0.f, 1.f);
      ospHdri.set("dir", 0.f, 1.f, 0.0f);
    }
    ospHdri.set( "intensity", HDRI_intensity);
    FileName imageFile(HDRI_file_name);
    ospray::miniSG::Texture2D *lightMap = ospray::miniSG::loadTexture(imageFile.path(), imageFile.base());
    if (lightMap == NULL){
      std::cout << "Failed to load hdri-light texture '" << imageFile << "'" << std::endl;
    } else {
      std::cout << "Successfully loaded hdri-light texture '" << imageFile << "'" << std::endl;
      OSPTexture2D ospLightMap = ospray::miniSG::createTexture2D(lightMap);
      ospHdri.set( "map", ospLightMap);
      ospHdri.commit();
      lights.push_back(ospHdri.handle());
    }
  }

  //TODO: Need to figure out where we're going to read lighting data from
  
  if (defaultDirLight_direction != vec3f(0.f)
      && defaultDirLight_intensity > 0.f) {
    auto ospLight = renderer.newLight("directional");
    if (ospLight.handle() == nullptr) {
      throw std::runtime_error("Failed to create a 'DirectionalLight'!");
    }
    ospLight.set("name", "sun");
    ospLight.set("color", 1.f, .94f, .88f);
    ospLight.set("direction", defaultDirLight_direction);
    ospLight.set("intensity", defaultDirLight_intensity);
    ospLight.set("angularDiameter", 0.53f);
    ospLight.commit();
    lights.push_back(ospLight.handle());
  }

  if (ambient.w > 0.f && reduce_max(ambient) > 0.f) {
    auto ospLight = renderer.newLight("ambient");
    if (ospLight.handle() == nullptr) {
      throw std::runtime_error("Failed to create a 'AmbientLight'!");
    }
    ospLight.set("name", "ambient");
    ospLight.set("color", ambient.x, ambient.y, ambient.z);
    ospLight.set("intensity", ambient.w);
    ospLight.commit();
    lights.push_back(ospLight.handle());
  }

  auto lightArray = ospray::cpp::Data(lights.size(), OSP_OBJECT, lights.data());
  //lightArray.commit();
  renderer.set("lights", lightArray);

  finalize();

  return true;
}

void DefaultLightsParser::finalize()
{
  
}
