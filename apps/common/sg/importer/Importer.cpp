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
#include "common/sg/SceneGraph.h"

/*! \file sg/module/Importer.cpp Defines the interface for writing
    file importers for the ospray::sg */

namespace ospray {
  namespace sg {

    using FileExtToImporterMap = std::map<const std::string, ImporterFunction>;

    FileExtToImporterMap importerForExtension;
    
    /*! declare an importer function for a given file extension */
    void declareImporterForFileExtension(const std::string &fileExtension,
                                         ImporterFunction importer)
    {
      importerForExtension[fileExtension] = importer;
    }

    /*! import a given file. throws a sg::RuntimeError if this could not be done */
    void importFile(std::shared_ptr<sg::World> &world, const FileName &fileName)
    {
      ImporterFunction importer = importerForExtension[fileName.ext()];
      if (importer) {
        ImportState state(world);
        importer(fileName,state);
      } else {
        throw sg::RuntimeError("unknown file format (fileName was '"
                               + fileName.str() + "')");
      }
    }

    void Importer::setChildrenModified(TimeStamp t)
    {
      Node::setChildrenModified(t);
      ospcommon::FileName file(child("fileName")->valueAs<std::string>());

      if (file.str() == loadedFileName)
        return;

      std::cout << "attempting importing file: " << file.str() << std::endl;

      if (loadedFileName != "" || file.str() == "")
        return; //TODO: support dynamic re-loading, need to clear children first

      loadedFileName = "";

      if (file.ext() == "obj") {
        std::cout << "importing file: " << file.str() << std::endl;
        sg::importOBJ(std::static_pointer_cast<sg::World>(shared_from_this()), file);
      } else if (file.ext() == "ply") {
        std::shared_ptr<sg::World> wsg(std::dynamic_pointer_cast<sg::World>(shared_from_this()));
        sg::importPLY(wsg, file);
      } else if (file.ext() == "osg" || file.ext() == "osp") {
        std::shared_ptr<sg::World> wsg(std::dynamic_pointer_cast<sg::World>(shared_from_this()));
        sg::loadOSP(file, wsg);
        instanced = false;
      } else {
        std::cout << "unsupported file format\n";
        return;
      }

      loadedFileName = file.str();
    }

    OSP_REGISTER_SG_NODE(Importer);

  }// ::ospray::sg
}// ::ospray


