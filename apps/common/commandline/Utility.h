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

#include <ospray_cpp/Camera.h>
#include <ospray_cpp/Model.h>
#include <ospray_cpp/Renderer.h>

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

using ParsedOSPObjects = std::tuple<ospcommon::box3f,
                                    ospray::cpp::Model,
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

/*

static OSPTexture2D createTexture2D(ospray::miniSG::Texture2D *msgTex)
{
  if(msgTex == nullptr)
  {
    static int numWarnings = 0;
    if (++numWarnings < 10)
    {
      std::cerr << "WARNING: material does not have Textures"
           << " (only warning for the first 10 times)!" << std::endl;
    }
    return nullptr;
  }

  static std::map<ospray::miniSG::Texture2D*,
      OSPTexture2D> alreadyCreatedTextures;
  if (alreadyCreatedTextures.find(msgTex) != alreadyCreatedTextures.end())
    return alreadyCreatedTextures[msgTex];

  //TODO: We need to come up with a better way to handle different possible
  //      pixel layouts
  OSPTextureFormat type = OSP_TEXTURE_R8;

  if (msgTex->depth == 1) {
    if( msgTex->channels == 1 ) type = OSP_TEXTURE_R8;
    if( msgTex->channels == 3 )
      type = msgTex->prefereLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
    if( msgTex->channels == 4 )
      type = msgTex->prefereLinear ? OSP_TEXTURE_RGBA8 : OSP_TEXTURE_SRGBA;
  } else if (msgTex->depth == 4) {
    if( msgTex->channels == 1 ) type = OSP_TEXTURE_R32F;
    if( msgTex->channels == 3 ) type = OSP_TEXTURE_RGB32F;
    if( msgTex->channels == 4 ) type = OSP_TEXTURE_RGBA32F;
  }

  OSPTexture2D ospTex = ospNewTexture2D(osp::vec2i{msgTex->width,
                                                   msgTex->height},
                                        type,
                                        msgTex->data);

  alreadyCreatedTextures[msgTex] = ospTex;

  ospCommit(ospTex);
  return ospTex;
}*/
