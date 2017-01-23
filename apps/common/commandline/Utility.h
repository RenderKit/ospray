// ======================================================================== //
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

#include <ospray/ospray_cpp/Camera.h>
#include <ospray/ospray_cpp/Model.h>
#include <ospray/ospray_cpp/Renderer.h>

#include "commandline/CameraParser.h"
#include "commandline/LightsParser.h"
#include "commandline/SceneParser/MultiSceneParser.h"
#include "commandline/RendererParser.h"

#include <tuple>
#include <type_traits>

inline void parseForLoadingModules(int ac, const char**& av)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--module" || arg == "-m") {
      ospLoadModule(av[++i]);
    }
  }
}

using ParsedOSPObjects = std::tuple<std::deque<ospcommon::box3f>,
                                    std::deque<ospray::cpp::Model>,
                                    ospray::cpp::Renderer,
                                    ospray::cpp::Camera>;

template <typename RendererParser_T,
          typename CameraParser_T,
          typename SceneParser_T,
          typename LightsParser_T>
inline ParsedOSPObjects parseCommandLine(int ac, const char **&av)
{
  static_assert(std::is_base_of<RendererParser, RendererParser_T>::value,
                "RendererParser_T is not a subclass of RendererParser.");
  static_assert(std::is_base_of<CameraParser, CameraParser_T>::value,
                "CameraParser_T is not a subclass of CameraParser.");
  static_assert(std::is_base_of<SceneParser, SceneParser_T>::value,
                "SceneParser_T is not a subclass of SceneParser.");
  static_assert(std::is_base_of<LightsParser, LightsParser_T>::value,
                "LightsParser_T is not a subclass of LightsParser.");

  parseForLoadingModules(ac, av);

  CameraParser_T cameraParser;
  cameraParser.parse(ac, av);
  auto camera = cameraParser.camera();

  RendererParser_T rendererParser;
  rendererParser.parse(ac, av);
  auto renderer = rendererParser.renderer();

  SceneParser_T sceneParser{rendererParser.renderer()};
  sceneParser.parse(ac, av);
  auto model = sceneParser.model();
  auto bbox  = sceneParser.bbox();

  LightsParser_T lightsParser(renderer);
  lightsParser.parse(ac, av);

  return std::make_tuple(bbox, model, renderer, camera);
}

inline ParsedOSPObjects parseWithDefaultParsers(int ac, const char**& av)
{
  return parseCommandLine<DefaultRendererParser, DefaultCameraParser,
                          MultiSceneParser, DefaultLightsParser>(ac, av);
}
