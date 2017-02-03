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

    // ==================================================================
    // sg node registry code
    // ==================================================================
    typedef std::shared_ptr<sg::Node> (*creatorFct)();
    
    std::map<std::string, creatorFct> sgNodeRegistry;

    /*! create a node of given type if registered (and tell it to
      parse itself from that xml node), or throw an exception if
      unkown node type */
    std::shared_ptr<sg::Node> createNodeFrom(const xml::Node &node, const unsigned char *binBasePtr)
    {
      std::map<std::string, creatorFct>::iterator it = sgNodeRegistry.find(node.name);
      creatorFct creator = NULL;
      if (it == sgNodeRegistry.end()) {
        const std::string creatorName = "ospray_create_sg_node__"+std::string(node.name);
        creator = (creatorFct)getSymbol(creatorName);
        if (!creator)
          throw std::runtime_error("unknown ospray scene graph node '"+node.name+"'");
        else
          std::cout << "#osp:sg: creating at least one instance of node type '" << node.name << "'" << std::endl;
        sgNodeRegistry[node.name] = creator;
      }
      else
        creator = it->second;
      
      assert(creator);
      std::shared_ptr<sg::Node> newNode = creator();
      if (!newNode)
        throw std::runtime_error("could not create scene graph node");
      
      newNode->setFromXML(node,binBasePtr);
      if (node.hasProp("name"))
        registerNamedNode(node.getProp("name"),newNode);
      return newNode;
    }

    // ==================================================================
    // XLM parser
    // ==================================================================

    std::shared_ptr<sg::Node> parseNode(const xml::Node &node);

    bool parseParam(std::shared_ptr<sg::Node> target, const xml::Node &node)
    {
      const std::string name = node.getProp("name");
      if (name == "") return false;
      if (node.name == "data") {
        assert(node.child.size() == 1);
        std::shared_ptr<sg::Node> value = parseNode(*node.child[0]);
        assert(value != NULL);
        std::shared_ptr<sg::DataBuffer> dataNode = std::dynamic_pointer_cast<sg::DataBuffer>(value);
        assert(dataNode);
        target->setParam(name,dataNode);
        // target->addParam(new ParamT<std::shared_ptr<DataBuffer> >(name,dataNode));
        return true;
      }
      if (node.name == "object") {
        assert(node.child.size() == 1);
        std::shared_ptr<sg::Node> value = parseNode(*node.child[0]);
        assert(value);
        target->setParam(name,value);
        // target->addParam(new ParamT<std::shared_ptr<sg::Node> >(name,value));
        return true;
      }
      if (node.name == "int") {
        // target->addParam(new ParamT<int32_t>(name,atoi(node.content.c_str())));
        target->setParam(name,std::stoi(node.content));
        return true;
      }
      if (node.name == "float") {
        // target->addParam(new ParamT<float>(name,atof(node.content.c_str())));
        target->setParam(name,std::stof(node.content));
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
    
    std::shared_ptr<sg::Integrator> parseIntegratorNode(const xml::Node &node)
    {
      assert(node.name == "Integrator");
      std::shared_ptr<Integrator> integrator = std::make_shared<Integrator>(node.getProp("type",""));
      for (auto c : node.child) {
        if (parseParam(std::dynamic_pointer_cast<sg::Node>(integrator),*c))
          continue;
        throw std::runtime_error("unknown node type '"+c->name
                                 +"' in ospray::sg::Integrator node");
      }
      return integrator;
    }
    
    void parseWorldNode(std::shared_ptr<sg::World> world,
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
      if (node.name == "Integrator")
        return parseIntegratorNode(node);
      std::cout << "warning: unknown sg::Node type '" << node.name << "'" << std::endl;
      return std::shared_ptr<sg::Node>();
    }

    std::shared_ptr<sg::World> importOSPVolumeViewerFile(std::shared_ptr<xml::XMLDoc> doc)
    {
      std::shared_ptr<sg::World> world = std::make_shared<sg::World>();
      std::shared_ptr<sg::StructuredVolumeFromFile> volume
        = std::make_shared<sg::StructuredVolumeFromFile>();

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
            std::cout << "#osp.sg: cowardly refusing to parse 'samplingRate'" << std::endl;
            std::cout << "#osp.sg: (note this should be OK)" << std::endl;
          } else
            throw std::runtime_error("unknown old-style osp file component volume::" + child.name);
        });
      std::cout << "#osp.sg: parsed old-style osp file as: " << std::endl;
      std::cout << "  fileName   = " << fileName << std::endl;
      std::cout << "  voxelType  = " << voxelType << std::endl;
      std::cout << "  dimensions = " << dimensions << std::endl;
      std::cout << "  path       = " << doc->fileName.path() << std::endl;
      volume->setTransferFunction(std::make_shared<TransferFunction>());
      volume->fileNameOfCorrespondingXmlDoc = doc->fileName;
      volume->setFileName(fileName);
      volume->setDimensions(dimensions);
      volume->setScalarType(voxelType);
      
      world->add(volume);
      return world;
    }

    std::shared_ptr<sg::World> loadOSP(const std::string &fileName)
    {
      std::shared_ptr<xml::XMLDoc> doc;
      // std::shared_ptr<xml::XMLDoc> doc = NULL;
      cout << "#osp:sg: starting to read OSPRay XML file '" << fileName << "'" << endl;
      doc = xml::readXML(fileName);
      cout << "#osp:sg: XML file read, starting to parse content..." << endl;
      assert(doc);

      if (doc->child.empty())
        throw std::runtime_error("ospray xml input file does not contain any nodes!?");

#if 1
      /* TEMPORARY FIX while we transition old volumeViewer .osp files
         (that are not in scene graph format) to actual scene graph */
      if (doc->child[0]->name == "volume") {
        std::cout << "#osp.sg: Seems the file we are parsing is actually not a " << endl;
        std::cout << "#osp.sg: ospray scene graph file, but rather a (older) " << endl;
        std::cout << "#osp.sg: ospVolumeViewer input file. Herocically trying to " << endl;
        std::cout << "#osp.sg: convert this to scene graph while loading. " << endl;
        return importOSPVolumeViewerFile(doc);
      }
#endif

      const std::string binFileName = fileName+"bin";
      const unsigned char * const binBasePtr = mapFile(binFileName);

      if (!doc) 
        throw std::runtime_error("could not parse "+fileName);
      
      if (doc->child.size() != 1)
        throw std::runtime_error("not an ospray xml file (empty XML document; no 'ospray' child node)'");
      if ((doc->child[0]->name != "ospray" && doc->child[0]->name != "OSPRay") )
        throw std::runtime_error("not an ospray xml file (document root node is '"+doc->child[0]->name+"', should be 'ospray'");

      std::shared_ptr<xml::Node> root = doc->child[0];
      std::shared_ptr<sg::World> world = std::make_shared<World>();//parseOSPRaySection(root->child[0]); 
      if (root->child.size() == 1 && root->child[0]->name == "World") {
        parseWorldNode(world,*root->child[0],binBasePtr);
      } else {
        parseWorldNode(world,*root,binBasePtr);
      }
      
      cout << "#osp:sg: done parsing OSP file" << endl;
      return world;
    }

  } // ::ospray::sg
} // ::ospray
