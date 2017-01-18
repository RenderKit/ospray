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

// ospray::sg
#include "Importer.h"

/*! \file sg/module/Importer.cpp Defines the interface for writing
    file importers for the ospray::sg */

namespace ospray {
  namespace sg {

    std::map<const std::string, ImporterFunction> *importerForExtension = nullptr;
    
    /*! declare an importer function for a given file extension */
    void declareImporterForFileExtension(const std::string &fileExtension,
                                         ImporterFunction importer)
    {
      if (!importerForExtension)
        importerForExtension = new std::map<const std::string, ImporterFunction>;
      (*importerForExtension)[fileExtension] = importer;
    }

    /*! import a given file. throws a sg::RuntimeError if this could not be done */
    void importFile(std::shared_ptr<sg::World> &world, const FileName &fileName)
    {
      if (!importerForExtension)
        importerForExtension = new std::map<const std::string, ImporterFunction>;
      ImporterFunction importer = (*importerForExtension)[fileName.ext()];
      if (importer) {
        ImportState state(world);
        importer(fileName,state);
      } else
        throw sg::RuntimeError("unknown file format (fileName was '"+fileName.str()+"')");
    }
    
  }
}


