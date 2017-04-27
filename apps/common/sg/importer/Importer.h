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

// ospray::sg
#include "../common/World.h"
#include "ospcommon/FileName.h"

namespace ospray {
  namespace sg {

    /*! helper class that parses a "points:///tmp/myfile.xyz:format=xyz:whatEver:radius=.3"
      file format url specifier into, in this example,

      formatType="points"
      fileName="/tmp/myFile.xyz"
      args={ {"format","xyz"}, {"whatEver",""}, {"radius",".3"} }
    */
    struct FormatURL 
    {
      /*! do the actual parsing, and return a formatURL */
      FormatURL(const std::string &input);

      /*! returns whether the given argument was specified in the format url */
      bool hasArg(const std::string &name) const;

      /*! return value of parameter with given name; returns "" if
          parameter wasn't supplied */
      std::string operator[](const std::string &name) const;

      /*! return value of parameter with given name; returns "" if
          parameter wasn't supplied */
      std::string operator[](const char *name) const;

      std::string formatType;
      std::string fileName;
      std::vector<std::pair<std::string,std::string>> args;
    };


    struct ImportState
    {
      ImportState(std::shared_ptr<sg::World> world)
        : world(world)
      {}

      std::shared_ptr<sg::World> world;
    };

    struct Importer : public sg::Renderable
    {
      Importer();

      virtual void setChildrenModified(TimeStamp t) override;

      std::string loadedFileName;
    };

    /*! prototype for any scene graph importer function */
    using ImporterFunction = void (*)(const FileName &fileName,
                                      sg::ImportState &importerState);
    
    /*! declare an importer function for a given file extension */
    void declareImporterForFileExtension(const std::string &fileExtension,
                                         ImporterFunction importer);

    /*! import a given file. throws a sg::RuntimeError if this could not be done */
    void importFile(std::shared_ptr<sg::World> &world, const FileName &fileName);

  } // ::ospray::sg
} // ::ospray


