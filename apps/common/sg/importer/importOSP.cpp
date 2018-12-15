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
#include "../common/NodeParameter.h"
#include "../common/NodeList.h"
// ospcommon
#include "ospcommon/containers/AlignedVector.h"
#include "ospcommon/utility/StringManip.h"

namespace ospray {
  namespace sg {

    static void importStructuredVolume(const std::shared_ptr<Node> &world,
                                       const xml::Node &xmlNode)
    {
      using SVFF = StructuredVolumeFromFile;
      auto volume = createNode("volume",
                               "StructuredVolumeFromFile")->nodeAs<SVFF>();

      vec3i dimensions(-1);
      vec3f gridSpacing = volume->child("gridSpacing").valueAs<vec3f>();
      std::string volumeFileName = "";
      std::string voxelType = "";

      containers::AlignedVector<vec4f> slices;

      for (const auto &child : xmlNode.child) {
        if (child.name == "dimensions")
          dimensions = toVec3i(child.content.c_str());
        else if (child.name == "voxelType")
          voxelType = child.content;
        else if (child.name == "filename")
          volumeFileName = child.content;
        else if (child.name == "slice") {
          auto components = utility::split(child.content, " ");
          if (components.size() != 4) {
            std::cerr << "WARNING: .osp files must have slices defined as 4 "
                      << "floats separated by spaces! Ignoring..." << std::endl;
            continue;
          }

          slices.emplace_back(
            std::atof(components[0].c_str()),
            std::atof(components[1].c_str()),
            std::atof(components[2].c_str()),
            std::atof(components[3].c_str())
          );
        } else if (child.name == "samplingRate") {
          // Silently ignore
        } else if (child.name == "gridSpacing") {
          gridSpacing = toVec3f(child.content.c_str());
        } else {
          throw std::runtime_error("unknown old-style osp file "
                                   "component volume::" + child.name);
        }
      }

      volume->fileNameOfCorrespondingXmlDoc = xmlNode.doc->fileName;
      volume->fileName = volumeFileName;

      volume->child("dimensions") = dimensions;
      volume->child("voxelType") = voxelType;
      volume->child("gridSpacing") = gridSpacing;

      world->add(volume);

      if (!slices.empty()) {
        // scale 4th component of slices by the dimension of the volume
        // (input value is on [0,1]), and add to scene
        for (auto &slice : slices)
          slice.w *= (dimensions * gridSpacing).x;

        auto slices_node = createNode("slices", "Slices");

#if 1 // enable for having values show up as widgets, disable for simple array
        auto slices_data = std::make_shared<NodeList<NodeParam<vec4f>>>();
        slices_data->setName("slices_list");

        for (size_t i = 0; i < slices.size(); ++i) {
          auto slice_node = std::make_shared<NodeParam<vec4f>>();
          slice_node->setName("slice" + std::to_string(i));
          slice_node->setType("vec4f");
          slice_node->setValue(slices[i]);
          slices_data->push_back(slice_node);
        }

        slices_node->add(slices_data);
        slices_node->setChild("volume", volume);
#else
        auto slices_data = std::make_shared<DataVector4f>();
        slices_data->v = slices;
        slices_data->setName("planes");

        slices_node->add(slices_data);
#endif

        slices_node->setChild("volume", volume);

        // add slices to world
        world->add(slices_node);
      }
    }

    static void importRAW2AMRVolume(const std::shared_ptr<Node> &world,
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
    static void importCHOMBOFromOSP(const std::shared_ptr<Node> &world,
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

    void loadOSP(const std::shared_ptr<Node> &world,
                 const std::string &fileName)
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
