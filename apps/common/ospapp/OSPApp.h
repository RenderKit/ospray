// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "ospcommon/utility/getEnvVar.h"

#include "common/sg/SceneGraph.h"

namespace ospray {
  namespace app {

    using namespace ospcommon;

    template <class T>
    class CmdLineParam
    {
     public:
      CmdLineParam(T defaultValue) { value = defaultValue; }
      T getValue() { return value; }

      CmdLineParam &operator=(const CmdLineParam &other)
      {
        value = other.value;
        overridden = true;
        return *this;
      }

      bool isOverridden() { return overridden; }

     private:
      T value;
      bool overridden = false;
    };

    class OSPApp
    {
     public:
      OSPApp() = default;
      virtual ~OSPApp() = default;

      int main(int argc, const char *argv[]);

     protected:

      virtual void render(const std::shared_ptr<sg::Frame> &) = 0;
      virtual int parseCommandLine(int &ac, const char **&av) = 0;

      int initializeOSPRay(int *argc, const char *argv[]);

      void addLightsToScene(sg::Node &renderer);
      void addImporterNodesToWorld(sg::Node &renderer);
      void addGeneratorNodesToWorld(sg::Node &renderer);
      void addAnimatedImporterNodesToWorld(sg::Node &renderer);
      void setupCamera(sg::Node &renderer);
      void setupToneMapping(sg::Node &fb, sg::Node &fb2);
      void addPlaneToScene(sg::Node &renderer);
      void printHelp();

      struct clTransform
      {
        vec3f translate{ 0, 0, 0 };
        vec3f scale{ 1, 1, 1 };
        vec3f rotation{ 0, 0, 0 };
      };

      // command line file
      struct clFile
      {
        clFile(std::string f, const clTransform &t) : file(f), transform(t) {}
        std::string file;
        clTransform transform;
      };

      struct clGeneratorCfg
      {
        std::string type;
        std::string params;
      };

      int width = 1024;
      int height = 768;
      CmdLineParam<vec3f> up = CmdLineParam<vec3f>({ 0, 1, 0 });
      CmdLineParam<vec3f> pos = CmdLineParam<vec3f>({ 0, 0, -1 });
      CmdLineParam<vec3f> gaze = CmdLineParam<vec3f>({ 0, 0, 0 });
      CmdLineParam<float> apertureRadius = CmdLineParam<float>(0.f);
      CmdLineParam<float> fovy = CmdLineParam<float>(60.f);

      std::vector<clFile> files;
      std::vector<clGeneratorCfg> generators;
      std::vector<std::vector<clFile>> animatedFiles;
      int matrix_i = 1, matrix_j = 1, matrix_k = 1;
      std::string hdriLightFile;
      bool addDefaultLights = false;
      bool noDefaultLights = false;
      bool aces = false;
      bool filmic = false;
      bool debug = false;
      std::string initialRendererType;
      box3f bboxWithoutPlane;
      std::vector<std::string> tfFiles;
      std::string defaultTransferFunction = "Jet";

      bool addPlane =
          utility::getEnvVar<int>("OSPRAY_APPS_GROUND_PLANE").value_or(0);

      bool fast =
          utility::getEnvVar<int>("OSPRAY_APPS_FAST_MODE").value_or(0);

     private:

      void parseGeneralCommandLine(int &ac, const char **&av);

      // parse command line arguments containing the format:
      //  -sg:nodeName:...:nodeName=value,value,value -- changes value
      //  -sg:nodeName:...:nodeName+=name,type        -- adds new child node
      void parseCommandLineSG(int ac, const char **&av, sg::Frame &root);
    };

  } // ::ospray::app
} // ::ospray
