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

#undef NDEBUG

// header
#include "SceneGraph.h"
#include "Importer.h"
// stl
#include <map>
// xml
#include "common/xml/XML.h"

namespace ospray {
  namespace sg {
    using std::string;
    using std::cout;
    using std::endl;

    /*! create a node of given type if registered (and tell it to
      parse itself from that xml node), or throw an exception if
      unkown node type */
    std::shared_ptr<sg::Node> createNodeFrom(const xml::Node &node,
                                             const unsigned char *binBasePtr)
    {
      if (node.name.find("Chombo") != std::string::npos) {
        if (!ospLoadModule("amr"))
            std::runtime_error("could not load amr module\n");
        if (!ospLoadModule("sg_amr"))
            std::runtime_error("could not load amr module\n");
      }

      std::string name = "untitled";
      if (node.hasProp("name"))
        name = node.getProp("name");

      std::shared_ptr<sg::Node> newNode = createNode(name, node.name);
      if (!newNode.get())
        throw std::runtime_error("could not create scene graph node");

      newNode->setFromXML(node,binBasePtr);

      return newNode;
    }

    // ==================================================================
    // XLM parser
    // ==================================================================

    std::shared_ptr<sg::Node> parseNode(const xml::Node &node);

    bool parseParam(std::shared_ptr<sg::Node> target, const xml::Node &node)
    {
      const std::string name = node.getProp("name");

      if (name.empty()) {
        return false;
      } else if (node.name == "data") {
        assert(node.child.size() == 1);
        std::shared_ptr<sg::Node> value = parseNode(*node.child[0]);
        assert(value != nullptr);
        std::shared_ptr<sg::DataBuffer> dataNode =
            std::dynamic_pointer_cast<sg::DataBuffer>(value);
        assert(dataNode);
        target->createChildWithValue(name,"data",dataNode);
        return true;
      } else if (node.name == "object") {
        assert(node.child.size() == 1);
        std::shared_ptr<sg::Node> value = parseNode(*node.child[0]);
        assert(value);
        target->createChildWithValue(name,"object",value);
        return true;
      } else if (node.name == "int") {
        target->createChildWithValue(name,"int",std::stoi(node.content));
        return true;
      } else if (node.name == "float") {
        target->createChildWithValue(name,"float",std::stof(node.content));
        return true;
      }

      return false;
    }

    std::shared_ptr<sg::Info> parseInfoNode(const xml::Node &node)
    {
      assert(node.name == "Info");
      std::shared_ptr<Info> info = std::make_shared<Info>();
      for (auto c : node.child) {
        if (c->name == "acks")
          info->acks = c->content;
        else if (c->name == "description")
          info->description = c->content;
        else if (c->name == "permissions")
          info->permissions = c->content;
        else
          throw std::runtime_error("unknown node type '"+c->name
                                   +"' in ospray::sg::Info node");
      }
      return info;
    }

    void parseWorldNode(std::shared_ptr<sg::Node> world,
                        const xml::Node &node,
                        const unsigned char *binBasePtr)
    {
      for (auto c : node.child) {
        std::shared_ptr<sg::Node> newNode = createNodeFrom(*c,binBasePtr);
        world->add(newNode);
      }
    }

    std::shared_ptr<sg::DataBuffer> parseDataNode(const xml::Node &node)
    {
#if 1
      NOTIMPLEMENTED;
#else
      assert(node.name == "Data");
      Data *data = new Data;
      assert(node.child.empty());
      size_t num = atol(node.getProp("num").c_str());
      size_t ofs = atol(node.getProp("ofs").c_str());
      return data;
#endif
    }

    std::shared_ptr<sg::Node> parseNode(const xml::Node &node)
    {
      if (node.name == "Data")
        return parseDataNode(node);
      if (node.name == "Info")
        return parseInfoNode(node);
      std::cerr << "Warning: unknown sg::Node type '" << node.name << "'"
                << std::endl;
      return std::shared_ptr<sg::Node>();
    }

    void importOSPVolumeViewerFile(std::shared_ptr<xml::XMLDoc> doc, std::shared_ptr<sg::Node> world)
    {
      std::shared_ptr<sg::StructuredVolumeFromFile> volume
        = std::dynamic_pointer_cast<sg::StructuredVolumeFromFile>(
          createNode("volume", "StructuredVolumeFromFile"));

      vec3i dimensions(-1);
      std::string fileName = "";
      std::string voxelType = "";
      xml::for_each_child_of(*doc->child[0],[&](const xml::Node &child){
          if (child.name == "dimensions")
            dimensions = toVec3i(child.content.c_str());
          else if (child.name == "voxelType")
            voxelType = child.content;
          else if (child.name == "filename")
            fileName = child.content;
          else if (child.name == "samplingRate") {
            std::cout << "#osp.sg: cowardly refusing to parse 'samplingRate'"
                      << std::endl;
            std::cout << "#osp.sg: (note this should be OK)" << std::endl;
          } else
            throw std::runtime_error("unknown old-style osp file component volume::"
                                     + child.name);
        });
      std::cout << "#osp.sg: parsed old-style osp file as: " << std::endl;
      std::cout << "  fileName   = " << fileName << std::endl;
      std::cout << "  voxelType  = " << voxelType << std::endl;
      std::cout << "  dimensions = " << dimensions << std::endl;
      std::cout << "  path       = " << doc->fileName.path() << std::endl;
      volume->fileNameOfCorrespondingXmlDoc = doc->fileName;
      volume->fileName = fileName;
      volume->dimensions = dimensions;
      volume->voxelType = voxelType;

      world->add(volume);
    }

    void loadOSP(std::shared_ptr<sg::Node> world, const std::string &fileName)
    {
      std::shared_ptr<xml::XMLDoc> doc;
      // std::shared_ptr<xml::XMLDoc> doc = NULL;
      cout << "#osp:sg: starting to read OSPRay XML file '" << fileName << "'" << endl;
      doc = xml::readXML(fileName);
      cout << "#osp:sg: XML file read, starting to parse content..." << endl;
      assert(doc);

      if (doc->child.empty())
        throw std::runtime_error("ospray xml input file does not contain any nodes!?");

      /* TEMPORARY FIX while we transition old volumeViewer .osp files
         (that are not in scene graph format) to actual scene graph */
      if (doc->child[0]->name == "volume") {
        std::cout << "#osp.sg: Seems the file we are parsing is actually not a " << endl;
        std::cout << "#osp.sg: ospray scene graph file, but rather an (older) " << endl;
        std::cout << "#osp.sg: ospVolumeViewer input file. Heroically trying to " << endl;
        std::cout << "#osp.sg: convert this to scene graph while loading. " << endl;
        importOSPVolumeViewerFile(doc, world);
        return;
      }

      const std::string binFileName = fileName+"bin";
      const unsigned char * const binBasePtr = mapFile(binFileName);

      if (!doc)
        throw std::runtime_error("could not parse "+fileName);

      if (doc->child.size() != 1)
      {
        throw std::runtime_error(
          "not an ospray xml file (empty XML document; no 'ospray' child node)'");
      }
      if ((doc->child[0]->name != "ospray" && doc->child[0]->name != "OSPRay") )
      {
        throw std::runtime_error("not an ospray xml file (document root node is '"
          +doc->child[0]->name+"', should be 'ospray'");
      }

      std::shared_ptr<xml::Node> root = doc->child[0];

      if (root->child.size() == 1 && root->child[0]->name == "World") {
        parseWorldNode(world,*root->child[0],binBasePtr);
      } else {
        parseWorldNode(world,*root,binBasePtr);
      }

      cout << "#osp:sg: done parsing OSP file" << endl;
    }

    std::shared_ptr<sg::Node> loadOSP(const std::string &fileName)
    {
      auto world = createNode("world", "World");
      loadOSP(std::static_pointer_cast<sg::Node>(world), fileName);

      return std::static_pointer_cast<sg::Node>(world);
    }

  } // ::ospray::sg
} // ::ospray
