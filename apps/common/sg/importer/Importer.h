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
    struct OSPSG_INTERFACE FormatURL
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


    struct OSPSG_INTERFACE ImportState
    {
      ImportState(std::shared_ptr<sg::World> world)
        : world(world)
      {}

      std::shared_ptr<sg::World> world;
    };

    struct OSPSG_INTERFACE Importer : public sg::Renderable
    {
      Importer();

      virtual void setChildrenModified(TimeStamp t) override;

      std::string loadedFileName;
    };

    /*! prototype for any scene graph importer function */
    using ImporterFunction = void (*)(const FileName &fileName,
                                      sg::ImportState &importerState);

    /*! declare an importer function for a given file extension */
    OSPSG_INTERFACE
    void declareImporterForFileExtension(const std::string &fileExtension,
                                         ImporterFunction importer);

    /*! import a given file. throws a sg::RuntimeError if this could
     *  not be done */
    OSPSG_INTERFACE
    void importFile(std::shared_ptr<sg::World> &world,
                    const FileName &fileName);

    /*! create a world from an already existing OSPModel */
    OSPSG_INTERFACE
    void importOSPModel(Node &world, OSPModel model,
                        const ospcommon::box3f &bbox);

    /*! import an OBJ wavefront model, and add its contents to the given
        world */
    OSPSG_INTERFACE void importOBJ(const std::shared_ptr<Node> &world,
                                   const FileName &fileName);

    /*! import an OSX streamlines model, and add its contents to the given
        world */
    OSPSG_INTERFACE void importOSX(const std::shared_ptr<Node> &world,
                                   const FileName &fileName);

    /*! import an PLY model, and add its contents to the given world */
    OSPSG_INTERFACE void importPLY(std::shared_ptr<Node> &world,
                                   const FileName &fileName);

#ifdef OSPRAY_APPS_SG_VTK
    OSPSG_INTERFACE void importTetVolume(const std::shared_ptr<Node> &world,
                                         const FileName &fileName);
#endif

    /*! import an X3D-format model, and add its contents to the given world */
    OSPSG_INTERFACE void importX3D(const std::shared_ptr<Node> &world,
                                   const FileName &fileName);

    OSPSG_INTERFACE void importXYZ(const std::shared_ptr<Node> &world,
                                   const FileName &fileName);

    /*! chombo amr */
    OSPSG_INTERFACE void importAMR(const FileName &fileName,
                                   sg::ImportState &importerState);

    OSPSG_INTERFACE
    std::shared_ptr<sg::Node> loadOSP(const std::string &fileName);

    OSPSG_INTERFACE
    void loadOSP(std::shared_ptr<sg::Node> world, const std::string &fileName);

    OSPSG_INTERFACE
    std::shared_ptr<sg::Node> readXML(const std::string &fileName);

    OSPSG_INTERFACE
    void importRIVL(std::shared_ptr<Node> world, const std::string &fileName);

    OSPSG_INTERFACE
    std::shared_ptr<sg::Node> loadOSG(const std::string &fileName);

    OSPSG_INTERFACE
    void loadOSPSG(const std::shared_ptr<Node> &world,
                   const std::string &fileName);

    OSPSG_INTERFACE
    void writeOSPSG(const std::shared_ptr<Node> &world,
                    const std::string &fileName);

  } // ::ospray::sg
} // ::ospray
