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
#include "common/sg/geometry/TriangleMesh.h"
#include "common/miniSG/miniSG.h"

/*! \file sg/module/Importer.cpp Defines the interface for writing
    file importers for the ospray::sg */

namespace ospray {
  namespace sg {

    // Helper functions ///////////////////////////////////////////////////////

    static inline void importMiniSg(miniSG::Model &msgModel,
                                    const ospcommon::FileName &fn)
    {
      if (fn.ext() == "stl")
        miniSG::importSTL(msgModel,fn);
      else if (fn.ext() == "msg")
        miniSG::importMSG(msgModel,fn);
      else if (fn.ext() == "tri")
        miniSG::importTRI_xyz(msgModel,fn);
      else if (fn.ext() == "xyzs")
        miniSG::importTRI_xyzs(msgModel,fn);
      else if (fn.ext() == "xml")
        miniSG::importRIVL(msgModel,fn);
      else if (fn.ext() == "hbp")
        miniSG::importHBP(msgModel,fn);
      else if (fn.ext() == "x3d")
        miniSG::importX3D(msgModel,fn);
    }

    // Importer definitions ///////////////////////////////////////////////////

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
        sg::loadOSP(wsg, file);
        instanced = false;
      } else if (file.ext() == "x3d" || file.ext() == "hbp" ||
                 file.ext() == "msg" || file.ext() == "stl" ||
                 file.ext() == "tri" || file.ext() == "xml") {

        miniSG::Model msgModel;
        importMiniSg(msgModel, file);

        for (auto mesh : msgModel.mesh) {
          auto sgMesh = std::dynamic_pointer_cast<sg::TriangleMesh>
            (createNode(mesh->name, "TriangleMesh").get());
          sgMesh->vertex = std::make_shared<DataVector3f>();
          for(int i =0; i < mesh->position.size(); i++)
            std::dynamic_pointer_cast<DataVector3f>(sgMesh->vertex)->push_back(
              mesh->position[i]);
          sgMesh->normal = std::make_shared<DataVector3f>();
          for(int i =0; i < mesh->normal.size(); i++)
            std::dynamic_pointer_cast<DataVector3f>(sgMesh->normal)->push_back(
              mesh->normal[i]);
          sgMesh->texcoord = std::make_shared<DataVector2f>();
          for(int i =0; i < mesh->texcoord.size(); i++)
            std::dynamic_pointer_cast<DataVector2f>(sgMesh->texcoord)->push_back(
              mesh->texcoord[i]);
          sgMesh->index =  std::make_shared<DataVector3i>();
          for(int i =0; i < mesh->triangle.size(); i++)
            std::dynamic_pointer_cast<DataVector3i>(sgMesh->index)->push_back(
              vec3i(mesh->triangle[i].v0, mesh->triangle[i].v1, mesh->triangle[i].v2));
          add(sgMesh);
        }
      } else {
        std::cout << "unsupported file format\n";
        return;
      }

      loadedFileName = file.str();
    }

    OSP_REGISTER_SG_NODE(Importer);

  }// ::ospray::sg
}// ::ospray


