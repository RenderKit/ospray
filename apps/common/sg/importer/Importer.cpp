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

// ospcommon
#include "ospcommon/utility/StringManip.h"
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
      std::cout << "#sg: declaring importer for '." << fileExtension
                << "' files" << std::endl;
      importerForExtension[fileExtension] = importer;
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

      std::cout << "attempting file import: " << fileName.str() << std::endl;

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
        importURL(wsg, fileName, *fu);
      } else if (utility::beginsWith(fileName, "--import:")) {
        auto splitValues = utility::split(fileName, ':');
        bool isGenerator = (splitValues.size() == 2);

        auto type = splitValues[1];

        if (isGenerator)
          importRegistryGenerator(wsg, type);
        else {
          auto file = splitValues[2];
          importRegistryFileLoader(wsg, type, FileName(file));
        }
      } else {
        importDefaultExtensions(wsg, fileName);
      }

      loadedFileName = fileName.str();
    }

    void Importer::importURL(std::shared_ptr<Node> world,
                             const FileName &fileName,
                             const FormatURL &fu) const
    {
      /* todo: move this code to a registry that automatically
         looks up right function based on loaded symbols... */
      if (fu.formatType == "points" || fu.formatType == "spheres") {
        importFileType_points(world, fileName);
      } else {
        std::cout << "Found a URL-style file type specified, but didn't recognize file type '"
                  << fu.formatType
                  << "' ... reverting to loading by file extension"
                  << std::endl;
      }
    }

    // TODO: Implement a registry for data "generators" which don't require
    //       an input file.
    void Importer::importRegistryGenerator(std::shared_ptr<Node> /*world*/,
                                           const std::string &/*type*/) const
    {
      #if 0
      // Function pointer type for creating a concrete instance of a subtype of
      // this class.
      using importFunction = void(*)(std::shared_ptr<Node>, const FileName &);

      // Function pointers corresponding to each subtype.
      static std::map<std::string, importFunction> symbolRegistry;

      // Find the creation function for the subtype if not already known.
      if (symbolRegistry.count(type) == 0) {
        postStatusMsg(2) << "#ospray: trying to look up "
                         << type_string << " type '" << type
                         << "' for the first time";

        // Construct the name of the creation function to look for.
        std::string creationFunctionName = "ospray_create_" + type_string
                                           +  "__" + type;

        // Look for the named function.
        symbolRegistry[type] =
            (creationFunctionPointer)getSymbol(creationFunctionName);

        // The named function may not be found if the requested subtype is not
        // known.
        if (!symbolRegistry[type]) {
          postStatusMsg(1) << "  WARNING: unrecognized " << type_string
                           << " type '" << type << "'.";
        }
      }

      // Create a concrete instance of the requested subtype.
      auto *object = symbolRegistry[type] ? (*symbolRegistry[type])() : nullptr;

      // Denote the subclass type in the ManagedObject base class.
      if (object) {
        object->managedObjectType = OSP_TYPE;
      }
      else {
        symbolRegistry.erase(type);
        throw std::runtime_error("Could not find " + type_string + " of type: "
          + type + ".  Make sure you have the correct OSPRay libraries linked.");
      }

      return object;
      #endif
    }

    void Importer::importRegistryFileLoader(std::shared_ptr<Node> world,
                                            const std::string &type,
                                            const FileName &fileName) const
    {
      using importFunction = void(*)(std::shared_ptr<Node>, const FileName &);

      static std::map<std::string, importFunction> symbolRegistry;

      if (symbolRegistry.count(type) == 0) {
        std::string creationFunctionName = "ospray_sg_import_" + type;
        symbolRegistry[type] =
            (importFunction)getSymbol(creationFunctionName);
      }

      auto fcn = symbolRegistry[type];

      if (fcn)
        fcn(world, fileName);
      else {
        symbolRegistry.erase(type);
        throw std::runtime_error("Could not find sg importer of type: "
          + type + ".  Make sure you have the correct libraries loaded.");
      }
    }

    static bool hasImporterForExtension(const std::string &fileExtension)
    {
      return importerForExtension.find(fileExtension)
          != importerForExtension.end();
    }

    void Importer::importDefaultExtensions(std::shared_ptr<Node> world,
                                           const FileName &fileName) const
    {
      auto ext = fileName.ext();

      if (hasImporterForExtension(ext)) {
        std::cout << "#sg: found importer for extension '" << ext << "'"
                  << std::endl;
        ImporterFunction importer = importerForExtension[ext];
        importer(world,fileName);
      } else if (ext == "obj") {
        sg::importOBJ(world, fileName);
      } else if (ext == "ply") {
        sg::importPLY(world, fileName);
      } else if (ext == "osg" || ext == "osp") {
        sg::loadOSP(world, fileName);
      } else if (ext == "osx") {
        sg::importOSX(world, fileName);
      } else if (ext == "xml") {
        sg::importRIVL(world, fileName);
      } else if (ext == "x3d") {
        sg::importX3D(world, fileName);
      } else if (ext == "xyz" || ext == "xyz2" || ext == "xyz3") {
        sg::importXYZ(world, fileName);
#ifdef OSPRAY_APPS_SG_VTK
      } else if (ext == "vtu" || ext == "vtk" || ext == "off") {
        sg::importUnstructuredVolume(world, fileName);
#endif
#ifdef OSPRAY_APPS_SG_CHOMBO
      } else if (ext == "hdf5") {
        sg::importCHOMBO(world, fileName);
#endif
      } else {
        std::cout << "unsupported file format\n";
        return;
      }
    }

    OSP_REGISTER_SG_NODE(Importer);

  }// ::ospray::sg
}// ::ospray


