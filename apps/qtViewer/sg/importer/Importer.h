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

// ospray::sg
#include "../common/World.h"
#include "ospcommon/FileName.h"

/*! \file sg/module/Importer.h Defines the interface for writing
    file importers for the ospray::sg */

namespace ospray {
  namespace sg {

    struct ImportState {
      Ref<sg::World> world;
      // std::vector<std::string> searchPaths;

      ImportState(Ref<sg::World> world)
        : world(world)
      {}
    };

    /*! prototype for any scene graph importer function */
    typedef void (*ImporterFunction)(const FileName &fileName,
                                     sg::ImportState &importerState);
    
    /*! declare an importer function for a given file extension */
    void declareImporterForFileExtension(const std::string &fileExtension,
                                         ImporterFunction importer);
    /*! import a given file. throws a sg::RuntimeError if this could not be done */
    void importFile(Ref<sg::World> &world, const FileName &fileName);

  }
}


