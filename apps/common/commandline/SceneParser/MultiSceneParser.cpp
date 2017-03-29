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

namespace commandline {

  using namespace ospray;
  using namespace ospcommon;

  MultiSceneParser::MultiSceneParser(cpp::Renderer renderer) 
    : renderer(renderer)
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
      sceneModels = parser->model();
      sceneBboxes = parser->bbox();
    } else {
      sceneModels.push_back(cpp::Model());
      sceneModels[0].commit();
    }

    /* iw, mar 17 - since the above code creates a single model for
       every file it loads we here merge them all together; the bette
       way would be to have a single model to contain several meshes -
       that would be faster _and_ cleaner - but that requires
       significant changes to how above code works ...  */
    if (sceneModels.size() > 1) {
      std::cout << "#msp: forced merging of multiple input models into a single model" << std::endl;
      ospray::cpp::Model jointModel;
      for (int i=0;i<sceneModels.size();i++) {
        ospcommon::affine3f xfm = ospcommon::one;
        ospray::cpp::Geometry asInstance = ospray::cpp::Geometry(ospNewInstance(sceneModels[i].handle(),(osp::affine3f&)xfm));
        jointModel.addGeometry(asInstance);
      }
      jointModel.commit();
      sceneModels.clear();
      sceneModels.push_back(jointModel);
    }
    if (sceneBboxes.size() > 1) {
      box3f mergedBounds = ospcommon::empty;
      for (auto box : sceneBboxes)
        mergedBounds.extend(box);
      sceneBboxes.clear();
      sceneBboxes.push_back(mergedBounds);
    }

    return parser != nullptr;
  }

  std::deque<cpp::Model> MultiSceneParser::model() const
  {
    return sceneModels;
  }

  std::deque<box3f> MultiSceneParser::bbox() const
  {
    return sceneBboxes;
  }

} // ::commandline
