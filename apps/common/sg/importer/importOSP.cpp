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

#include "Importer.h"

#include "../volume/AMRVolume.h"

#include "ospcommon/utility/StringManip.h"

namespace ospray {
  namespace sg {

    static void importStructuredVolume(std::shared_ptr<Node> world,
                                       const xml::Node &xmlNode)
    {
      using SVFF = StructuredVolumeFromFile;
      auto volume = createNode("volume",
                               "StructuredVolumeFromFile")->nodeAs<SVFF>();

      vec3i dimensions(-1);
      std::string volumeFileName = "";
      std::string voxelType = "";

      for (const auto &child : xmlNode.child) {
        if (child.name == "dimensions")
          dimensions = toVec3i(child.content.c_str());
        else if (child.name == "voxelType")
          voxelType = child.content;
        else if (child.name == "filename")
          volumeFileName = child.content;
        else if (child.name == "samplingRate") {
          // Silently ignore
        } else {
          throw std::runtime_error("unknown old-style osp file "
                                   "component volume::" + child.name);
        }
      }

      volume->fileNameOfCorrespondingXmlDoc = xmlNode.doc->fileName;
      volume->fileName = volumeFileName;
      volume->dimensions = dimensions;
      volume->voxelType = voxelType;

      world->add(volume);
    }

    static void importRAW2AMRVolume(std::shared_ptr<Node> world,
                                    const std::string &originalFileName,
                                    const xml::Node &xmlNode)
    {
      FileName orgFile(originalFileName);

      auto node = sg::createNode("amr", "AMRVolume")->nodeAs<sg::AMRVolume>();
      std::string fileName;
      int brickSize = -1;

      for (const auto &child : xmlNode.child) {
        if (child.name == "brickSize")
          brickSize = std::atoi(child.content.c_str());
        else if (child.name == "fileName")
          fileName = orgFile.path() + child.content;
      }

      if (fileName == "") {
        throw std::runtime_error("no child element 'fileName' specified "
                                 "for AMR volume!");
      } else if (brickSize == -1) {
        throw std::runtime_error("no child element 'brickSize' specified "
                                 "for AMR volume!");
      }

      node->parseRaw2AmrFile(fileName, brickSize);

      world->add(node);
    }

#ifdef OSPRAY_APPS_SG_CHOMBO
    static void importCHOMBOFromOSP(std::shared_ptr<Node> world,
                                    const std::string &originalFileName,
                                    const xml::Node &xmlNode)
    {
      FileName orgFile(originalFileName);

      std::string fileName;

      for (const auto &child : xmlNode.child) {
        if (child.name == "fileName")
          fileName = orgFile.path() + child.content;
      }

      if (fileName == "") {
        throw std::runtime_error("no child element 'fileName' specified "
                                 "for AMR volume!");
      }

      importCHOMBO(world, fileName);
    }
#endif

    void loadOSP(std::shared_ptr<Node> world, const std::string &fileName)
    {
      std::cout << "#osp:sg: starting to read OSPRay XML file '" << fileName
                << "'" << std::endl;

      auto doc = xml::readXML(fileName);

      std::cout << "#osp:sg: XML file read, starting to parse content..."
                << std::endl;

      if (doc->child.empty())
        throw std::runtime_error("ospray xml input file does not contain any nodes!?");

      for (const auto &node : doc->child) {
        auto nameLower = utility::lowerCase(node.name);
        if (nameLower == "volume" || nameLower == "structuredvolume")
          importStructuredVolume(world, node);
        else if (nameLower == "amr" || nameLower == "amrvolume")
          importRAW2AMRVolume(world, fileName, node);
#ifdef OSPRAY_APPS_SG_CHOMBO
        else if (nameLower == "chombo" || nameLower == "chombovolume")
          importCHOMBOFromOSP(world, fileName, node);
#endif
        else
          std::cout << "#importOSG: unknown xml tag '" << node.name << "'\n";
      }
    }

  } // ::ospray::sg
} // ::ospray
