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

/*! \file sg/module/Importer.cpp Defines the interface for writing
  file importers for the ospray::sg */

namespace ospray {
  namespace sg {


    struct AutoFree
    {
      AutoFree(void *s) : s(s) {}
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

      char *fName = urlSep+3;
      char *arg = strtok(fName,":");
      this->fileName = fName;

      arg = strtok(nullptr,":");
      std::vector<std::string> local_args;
      while (arg) {
        local_args.push_back(arg);
        arg = strtok(nullptr,":");
      }

      // now, parse all name:value pairs
      for (auto arg_i : local_args) {
        char *s = strdup(arg_i.c_str());
        AutoFree _s(s);
        char *name = strtok(s,"=");
        char *val  = strtok(nullptr,"=");
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
    void importFileType_points(std::shared_ptr<Node> &world,
                               const FileName &url);



    // Importer definitions ///////////////////////////////////////////////////

    using FileExtToImporterMap = std::map<const std::string, ImporterFunction>;

    static FileExtToImporterMap importerForExtension;

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
      createChild("fileName", "string");
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

      auto wsg = this->nodeAs<Node>();

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

      auto ext = fileName.ext();

      if (ext == "obj") {
        sg::importOBJ(wsg, fileName);
      } else if (ext == "ply") {
        sg::importPLY(wsg, fileName);
      } else if (ext == "osg" || ext == "osp") {
        sg::loadOSP(wsg, fileName);
      } else if (ext == "osx") {
        sg::importOSX(wsg, fileName);
      } else if (ext == "xml") {
        sg::importRIVL(wsg, fileName);
      } else if (ext == "xyz" || ext == "xyz2" || ext == "xyz3") {
        sg::importXYZ(wsg, fileName);
#ifdef OSPRAY_APPS_SG_VTK
      } else if (ext == "vtu" || ext == "off") {
        sg::importTetVolume(wsg, fileName);
#endif
      } else {
        std::cout << "unsupported file format\n";
        return;
      }

      loadedFileName = fileName.str();
    }

    OSP_REGISTER_SG_NODE(Importer);

  }// ::ospray::sg
}// ::ospray


