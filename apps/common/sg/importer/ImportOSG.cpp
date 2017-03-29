// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
    typedef sg::Node *(*creatorFct)();
    
    std::map<std::string, creatorFct> osgNodeRegistry;

    /*! create a node of given type if registered (and tell it to
      parse itself from that xml node), or throw an exception if
      unkown node type */
    // sg::Node *createSGNodeFrom(const xml::Node &node, const unsigned char *binBasePtr)
    // {
    //   std::map<std::string, creatorFct>::iterator it = osgNodeRegistry.find(node.name);
    //   creatorFct creator = NULL;
    //   if (it == osgNodeRegistry.end()) {
    //     std::string creatorName = "ospray_create_sg_node__"+std::string(node.name);
    //     creator = (creatorFct)getSymbol(creatorName);
    //     if (!creator)
    //       throw std::runtime_error("unknown ospray scene graph node '"+node.name+"'");
    //     else
    //       std::cout << "#osp:sg: creating at least one instance of node type '" << node.name << "'" << std::endl;
    //     osgNodeRegistry[node.name] = creator;
    //   } else creator = it->second;
    //   assert(creator);
    //   sg::Node *newNode = creator();
    //   assert(newNode);
    //   newNode->setType(node.name);
    //   newNode->setName(node.getProp("name"));
    //   try {
    //     newNode->setFromXML(node,binBasePtr);
    //     return newNode;
    //   } catch (std::runtime_error e) {
    //     delete newNode;
    //     throw e;
    //   }
    // }

    // ==================================================================
    // XLM parser
    // ==================================================================

    sg::Node *parseSGNode(const xml::Node &node);

    bool parseSGParam(sg::Node *target, const xml::Node &node)
    {
      const std::string name = node.getProp("name");
      if (name == "") return false;
      if (node.name == "data") {
        assert(node.child.size() == 1);
        sg::Node *value = parseSGNode(*node.child[0]);
        assert(value != NULL);
        std::shared_ptr<sg::Node> dataNode(value);
        assert(dataNode);
        target->setParam(name,dataNode);
        // target->addParam(new ParamT<Ref<DataBuffer> >(name,dataNode));
        return true;
      }
      if (node.name == "object") {
        assert(node.child.size() == 1);
        std::shared_ptr<sg::Node> value = std::shared_ptr<Node>(parseSGNode(*node.child[0]));
        assert(value);
        target->setParam(name,value);
        // target->addParam(new ParamT<Ref<sg::Node> >(name,value));
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

    sg::Info *parseSGInfoNode(const xml::Node &node)
    {
      assert(node.name == "Info");
      Info *info = new Info;
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
    
    sg::Integrator *parseSGIntegratorNode(const xml::Node &node)
    {
      assert(node.name == "Integrator");
      Integrator *integrator = new Integrator(node.getProp("type",""));
      for (auto c : node.child) {
        if (parseSGParam(integrator,*c))
          continue;
        throw std::runtime_error("unknown node type '"+c->name
                                 +"' in ospray::sg::Integrator node");
      }
      return integrator;
    }

    void parseSGWorldNode(sg::World *world,
                        const xml::Node &node,
                        const unsigned char *binBasePtr)
    {
      for (auto c : node.child) {
        //TODO: Carson: better way to do this?
        std::cout << "loading node: " << c->name << std::endl;
        if (c->name.find("Chombo") != std::string::npos)
        {
          std::cout << "loading amr\n";
          ospLoadModule("amr"); 
          ospLoadModule("sg_amr");  
        }
        std::shared_ptr<sg::Node> newNode =
            createNode(c->getProp("name"),c->name).get();
        newNode->setFromXML(*c,binBasePtr);
        world->nodes.push_back(newNode);
        world->add(newNode);
        std::cout << "adding node to world: " << newNode->name() << " "
                  << newNode->type() << "\n";
      }
    }
    
    sg::DataBuffer *parseSGDataNode(const xml::Node &node)
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

    sg::Node *parseSGNode(const xml::Node &node)
    {
      if (node.name == "Data")
        return parseSGDataNode(node);
      if (node.name == "Info")
        return parseSGInfoNode(node);
      if (node.name == "Integrator")
        return parseSGIntegratorNode(node);
      std::cout << "warning: unknown sg::Node type '" << node.name << "'" << std::endl;
      return NULL;
    }

    std::shared_ptr<sg::World> loadOSG(const std::string &fileName)
    {
      std::shared_ptr<xml::XMLDoc> doc;
      // Ref<xml::XMLDoc> doc = NULL;
      cout << "#osp:sg: starting to read OSPRay XML file '" << fileName << "'" << endl;
      doc = xml::readXML(fileName);
      cout << "#osp:sg: XML file read, starting to parse content..." << endl;

      const std::string binFileName = fileName+"bin";
      const unsigned char * const binBasePtr = mapFile(binFileName);

      if (!doc) 
        throw std::runtime_error("could not parse "+fileName);
      
      if (doc->child.size() != 1)
        throw std::runtime_error("not an ospray xml file (empty XML document; no 'ospray' child node)'");
      if ((doc->child[0]->name != "ospray" && doc->child[0]->name != "OSPRay") )
        throw std::runtime_error("not an ospray xml file (document root node is '"+doc->child[0]->name+"', should be 'ospray'");

      std::shared_ptr<xml::Node> root = doc->child[0];
      std::shared_ptr<sg::World> world(new World);//parseOSPRaySection(root->child[0]); 
      if (root->child.size() == 1 && root->child[0]->name == "World") {
        parseSGWorldNode(world.get(),*root->child[0],binBasePtr);
      } else {
        parseSGWorldNode(world.get(),*root,binBasePtr);
      }
      
      cout << "#osp:sg: done parsing OSP file" << endl;
      return world;
    }

    void loadOSG(const std::string &fileName, std::shared_ptr<sg::World> world)
    {
      std::shared_ptr<xml::XMLDoc> doc;
      // Ref<xml::XMLDoc> doc = NULL;
      cout << "#osp:sg: starting to read OSPRay XML file '" << fileName << "'" << endl;
      doc = xml::readXML(fileName);
      cout << "#osp:sg: XML file read, starting to parse content..." << endl;

      const std::string binFileName = fileName+"bin";
      const unsigned char * const binBasePtr = mapFile(binFileName);

      if (!doc) 
        throw std::runtime_error("could not parse "+fileName);
      
      if (doc->child.size() != 1)
        throw std::runtime_error("not an ospray xml file (empty XML document; no 'ospray' child node)'");
      if ((doc->child[0]->name != "ospray" && doc->child[0]->name != "OSPRay") )
        throw std::runtime_error("not an ospray xml file (document root node is '"+doc->child[0]->name+"', should be 'ospray'");

      std::shared_ptr<xml::Node> root = doc->child[0];
      // std::shared_ptr<sg::World> world(new World);//parseOSPRaySection(root->child[0]); 
      if (root->child.size() == 1 && root->child[0]->name == "World") {
        parseSGWorldNode(world.get(),*root->child[0],binBasePtr);
      } else {
        parseSGWorldNode(world.get(),*root,binBasePtr);
      }
      
      cout << "#osp:sg: done parsing OSP file" << endl;
      // return world;
    }

  } // ::ospray::sg
} // ::ospray
