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


    struct AutoFree
    {
      AutoFree(void *s) : s(s) {};
      ~AutoFree() { free(s); }
      void *s;
    };


    /*! do the actual parsing, and return a formatURL */
    FormatURL::FormatURL(const std::string &input)
    {
      char *buffer = strdup(input.c_str());
      AutoFree _buffer(buffer);
        
      char *urlSep = strstr(buffer,"://");
      if (!urlSep) 
        throw std::runtime_error("not actually a file format url");

      *urlSep = 0;
      this->formatType = buffer;

      char *fileName = urlSep+3;
      char *arg = strtok(fileName,":");
      this->fileName = fileName;

      arg = strtok(NULL,":");
      std::vector<std::string> args;
      while (arg) {
        args.push_back(arg);
        arg = strtok(NULL,":");
      }

      // now, parse all name:value pairs
      for (auto arg_i : args) {
        char *s = strdup(arg_i.c_str());
        AutoFree _s(s);
        char *name = strtok(s,"=");
        char *val  = strtok(NULL,"=");
        std::pair<std::string,std::string> newArg(name,val?val:"");
        this->args.push_back(newArg);
      }
    }

    /*! returns whether the given argument was specified in the format url */
    bool FormatURL::hasArg(const std::string &name) const
    { 
      for (auto &a : args)
        if (a.first == name) return true;
      return false; 
    }

    /*! return value of parameter with given name; returns "" if
      parameter wasn't supplied */
    std::string FormatURL::operator[](const std::string &name) const
    { 
      for (auto &a : args)
        if (a.first == name) return a.second;
      return std::string("<invalid parameter name>"); 
    }

    /*! return value of parameter with given name; returns "" if
      parameter wasn't supplied */
    std::string FormatURL::operator[](const char *name) const
    { 
      return (*this)[std::string(name)]; 
    }




    // for now, let's hardcode the importers - should be moved to a
    // registry at some point ...
    void importFileType_points(std::shared_ptr<World> &world,
                               const FileName &url);



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

    Importer::Importer()
    {
      createChildNode("fileName", "string");
    }

    void Importer::setChildrenModified(TimeStamp t)
    {
      Node::setChildrenModified(t);
      ospcommon::FileName fileName(child("fileName").valueAs<std::string>());

      if (fileName.str() == loadedFileName)
        return;

      std::cout << "attempting importing file: " << fileName.str() << std::endl;

      if (loadedFileName != "" || fileName.str() == "")
        return; //TODO: support dynamic re-loading, need to clear children first

      loadedFileName = "";

      std::shared_ptr<sg::World> wsg(std::dynamic_pointer_cast<sg::World>(shared_from_this()));

#if 1
      std::shared_ptr<FormatURL> fu;
      try {
        fu = std::make_shared<FormatURL>(fileName.c_str());
      } catch (std::runtime_error e) {
        /* this failed so this was not a file type url ... */
        fu = nullptr;
      }

      if (fu) {
        // so this _was_ a file type url

        /* todo: move this code to a registry that automatically
           looks up right function based on loaded symbols... */
        if (fu->formatType == "points" || fu->formatType == "spheres") {
          importFileType_points(wsg,fileName);
          loadedFileName = fileName;
          return;
        } else
          std::cout << "Found a URL-style file type specified, but didn't recognize file type '" << fu->formatType<< "' ... reverting to loading by file extension" << std::endl;
      } 
#endif
      if (fileName.ext() == "obj") {
        std::cout << "importing file: " << fileName.str() << std::endl;
        sg::importOBJ(std::static_pointer_cast<sg::World>(shared_from_this()), fileName);
      } else if (fileName.ext() == "ply") {
        sg::importPLY(wsg, fileName);
      } else if (fileName.ext() == "osg" || fileName.ext() == "osp") {
        sg::loadOSP(wsg, fileName);
        instanced = false;
      } else if (fileName.ext() == "x3d" || fileName.ext() == "hbp" ||
                 fileName.ext() == "msg" || fileName.ext() == "stl" ||
                 fileName.ext() == "tri" || fileName.ext() == "xml") {

        miniSG::Model msgModel;
        importMiniSg(msgModel, fileName);

        for (auto mesh : msgModel.mesh) {
          auto sgMesh = std::dynamic_pointer_cast<sg::TriangleMesh>
            (createNode(mesh->name, "TriangleMesh"));
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
    
      loadedFileName = fileName.str();
    }

    OSP_REGISTER_SG_NODE(Importer);

  }// ::ospray::sg
}// ::ospray


