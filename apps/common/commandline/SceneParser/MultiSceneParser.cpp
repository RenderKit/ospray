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

#include "MultiSceneParser.h"

#include "particle/ParticleSceneParser.h"
#include "streamlines/StreamLineSceneParser.h"
#ifdef OSPRAY_TACHYON_SUPPORT
#  include "tachyon/TachyonSceneParser.h"
#endif
#include "trianglemesh/TriangleMeshSceneParser.h"
#ifndef _WIN32
# include "volume/VolumeSceneParser.h"
#endif

using namespace ospray;
using namespace ospcommon;

MultiSceneParser::MultiSceneParser(cpp::Renderer renderer) :
  renderer(renderer)
{
}

bool MultiSceneParser::parse(int ac, const char **&av)
{
  TriangleMeshSceneParser triangleMeshParser(renderer);
#ifdef OSPRAY_TACHYON_SUPPORT
  TachyonSceneParser      tachyonParser(renderer);
#endif
  ParticleSceneParser     particleParser(renderer);
  StreamLineSceneParser   streamlineParser(renderer);
#ifndef _WIN32
  VolumeSceneParser       volumeParser(renderer);
#endif

  bool gotTriangleMeshScene = triangleMeshParser.parse(ac, av);
#ifdef OSPRAY_TACHYON_SUPPORT
  bool gotTachyonScene      = tachyonParser.parse(ac, av);
#endif
  bool gotParticleScene      = particleParser.parse(ac, av);
  bool gotStreamLineScene   = streamlineParser.parse(ac, av);
#ifndef _WIN32
  bool gotVolumeScene       = volumeParser.parse(ac, av);
#endif

  SceneParser *parser = nullptr;

  if (gotTriangleMeshScene)
    parser = &triangleMeshParser;
#ifdef OSPRAY_TACHYON_SUPPORT
  else if (gotTachyonScene)
    parser = &tachyonParser;
#endif
  else if (gotParticleScene)
    parser = &particleParser;
  else if (gotStreamLineScene)
    parser = &streamlineParser;
#ifndef _WIN32
  else if (gotVolumeScene)
    parser = &volumeParser;
#endif

  if (parser) {
    sceneModel = parser->model();
    sceneBbox  = parser->bbox();
  } else {
    sceneModel = cpp::Model();
    sceneModel.commit();
  }

  return parser != nullptr;
}

cpp::Model MultiSceneParser::model() const
{
  return sceneModel;
}

box3f MultiSceneParser::bbox() const
{
  return sceneBbox;
}
