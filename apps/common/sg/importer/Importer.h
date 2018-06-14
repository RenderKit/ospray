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

// ospray::sg
#include "../common/Model.h"
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


    struct OSPSG_INTERFACE Importer : public sg::Renderable
    {
      Importer();

      virtual void setChildrenModified(TimeStamp t) override;

      std::string loadedFileName;

    private:

      void importURL(const std::shared_ptr<Node> &world,
                     const FileName &fileName,
                     const FormatURL &fu) const;
      void importRegistryFileLoader(const std::shared_ptr<Node> &world,
                                    const std::string &type,
                                    const FileName &fileName) const;
      void importDefaultExtensions(const std::shared_ptr<Node> &world,
                                   const FileName &filename);
    };

    /*! prototype for any scene graph importer function */
    using ImporterFunction = void (*)(const std::shared_ptr<Node> &world,
                                      const FileName &fileName);

    /*! declare an importer function for a given file extension */
    OSPSG_INTERFACE
    void declareImporterForFileExtension(const std::string &fileExtension,
                                         ImporterFunction importer);

    /*! import an OBJ wavefront model, and add its contents to the given
        world */
    OSPSG_INTERFACE void importOBJ(const std::shared_ptr<Node> &world,
                                   const FileName &fileName);

    /*! import an X3D model, as forexample a ParaView contour exported
      using ParaView's X3D exporter */
    OSPSG_INTERFACE void importX3D(const std::shared_ptr<Node> &world,
                                   const FileName &fileName);

    /*! import an OSX streamlines model, and add its contents to the given
        world */
    OSPSG_INTERFACE void importOSX(const std::shared_ptr<Node> &world,
                                   const FileName &fileName);

    /*! import an PLY model, and add its contents to the given world */
    OSPSG_INTERFACE void importPLY(const std::shared_ptr<Node> &world,
                                   const FileName &fileName);

#ifdef OSPRAY_APPS_SG_VTK
    OSPSG_INTERFACE
    void importUnstructuredVolume(const std::shared_ptr<Node> &world,
                                  const FileName &fileName);

    OSPSG_INTERFACE void importVTKPolyData(const std::shared_ptr<Node> &world,
                                           const FileName &fileName);

    OSPSG_INTERFACE void importVTI(const std::shared_ptr<Node> &world,
                                   const FileName &fileName);

    OSPSG_INTERFACE void importVTIs(const std::shared_ptr<Node> &world,
                                    const std::vector<FileName> &fileNames);
#endif

    /*! import an X3D-format model, and add its contents to the given world */
    OSPSG_INTERFACE void importX3D(const std::shared_ptr<Node> &world,
                                   const FileName &fileName);

    OSPSG_INTERFACE void importXYZ(const std::shared_ptr<Node> &world,
                                   const FileName &fileName);

#ifdef OSPRAY_APPS_SG_CHOMBO
    /*! chombo amr */
    OSPSG_INTERFACE void importCHOMBO(const std::shared_ptr<Node> &world,
                                      const FileName &fileName);
#endif

    OSPSG_INTERFACE
    void loadOSP(const std::shared_ptr<Node> &world,
                 const std::string &fileName);

    OSPSG_INTERFACE
    std::shared_ptr<Node> readXML(const std::string &fileName);

    OSPSG_INTERFACE
    void importRIVL(const std::shared_ptr<Node> &world,
                    const FileName &fileName);

    OSPSG_INTERFACE
    std::shared_ptr<Node> loadOSG(const std::string &fileName);

    OSPSG_INTERFACE
    void loadOSPSG(const std::shared_ptr<Node> &world,
                   const std::string &fileName);

    OSPSG_INTERFACE
    void writeOSPSG(const std::shared_ptr<Node> &world,
                    const std::string &fileName);


    // Macro to register importers ////////////////////////////////////////////

#define OSPSG_REGISTER_IMPORT_FUNCTION(function, name)                         \
    extern "C" OSPRAY_DLLEXPORT                                                \
    void ospray_sg_import_##name(const std::shared_ptr<Node> &world,           \
                                 const FileName fileName)                      \
    {                                                                          \
      function(world, fileName);                                               \
    }                                                                          \
    /* additional declaration to avoid "extra ;" -Wpedantic warnings */        \
    void ospray_sg_import_##name()

  } // ::ospray::sg
} // ::ospray
