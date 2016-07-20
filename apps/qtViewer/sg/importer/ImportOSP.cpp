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
    
    std::map<std::string, creatorFct> sgNodeRegistry;

    /*! create a node of given type if registered (and tell it to
      parse itself from that xml node), or throw an exception if
      unkown node type */
    sg::Node *createNodeFrom(const xml::Node *node, const unsigned char *binBasePtr)
    {
      std::map<std::string, creatorFct>::iterator it = sgNodeRegistry.find(node->name);
      creatorFct creator = NULL;
      if (it == sgNodeRegistry.end()) {
        std::string creatorName = "ospray_create_sg_node__"+std::string(node->name);
        creator = (creatorFct)getSymbol(creatorName);
        if (!creator)
          throw std::runtime_error("unknown ospray scene graph node '"+node->name+"'");
        else
          std::cout << "#osp:sg: creating at least one instance of node type '" << node->name << "'" << std::endl;
        sgNodeRegistry[node->name] = creator;
      } else creator = it->second;
      assert(creator);
      sg::Node *newNode = creator();
      assert(newNode);
      try {
        newNode->setFromXML(node,binBasePtr);
        return newNode;
      } catch (std::runtime_error e) {
        delete newNode;
        throw e;
      }
    }

    // ==================================================================
    // XLM parser
    // ==================================================================

    sg::Node *parseNode(xml::Node *node);

    bool parseParam(sg::Node *target, xml::Node *node)
    {
      const std::string name = node->getProp("name");
      if (name == "") return false;
      if (node->name == "data") {
        assert(node->child.size() == 1);
        sg::Node *value = parseNode(node->child[0]);
        assert(value != NULL);
        sg::DataBuffer *dataNode = dynamic_cast<sg::DataBuffer *>(value);
        assert(dataNode);
        target->addParam(new ParamT<Ref<DataBuffer> >(name,dataNode));
        return true;
      }
      if (node->name == "object") {
        assert(node->child.size() == 1);
        sg::Node *value = parseNode(node->child[0]);
        assert(value != NULL);
        target->addParam(new ParamT<Ref<sg::Node> >(name,value));
        return true;
      }
      if (node->name == "int") {
        target->addParam(new ParamT<int32_t>(name,atoi(node->content.c_str())));
        return true;
      }
      if (node->name == "float") {
        target->addParam(new ParamT<float>(name,atof(node->content.c_str())));
        return true;
      }
      return false;
    }

    sg::Info *parseInfoNode(xml::Node *node)
    {
      assert(node->name == "Info");
      Info *info = new Info;
      for (int i=0;i<node->child.size();i++) {
        xml::Node *c = node->child[i];
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
    
    // sg::Geometry *parseGeometryNode(xml::Node *node)
    // {
    //   assert(node->name == "Geometry");
    //   Geometry *geometry = new Geometry(node->getProp("type"));
    //   for (int i=0;i<node->child.size();i++) {
    //     xml::Node *c = node->child[i];
    //     if (parseParam(geometry,c))
    //       continue;
    //     throw std::runtime_error("unknown node type '"+c->name
    //                              +"' in ospray::sg::Geometry node");
    //   }
    //   return geometry;
    // }
    
    sg::Integrator *parseIntegratorNode(xml::Node *node)
    {
      assert(node->name == "Integrator");
      Integrator *integrator = new Integrator(node->getProp("type"));
      for (int i=0;i<node->child.size();i++) {
        xml::Node *c = node->child[i];
        if (parseParam(integrator,c))
          continue;
        throw std::runtime_error("unknown node type '"+c->name
                                 +"' in ospray::sg::Integrator node");
      }
      return integrator;
    }
    
    void parseWorldNode(sg::World *world, xml::Node *node, const unsigned char *binBasePtr)
    {
      for (int i=0;i<node->child.size();i++) {
        xml::Node *c = node->child[i];
        Ref<sg::Node> newNode = createNodeFrom(c,binBasePtr);
        world->node.push_back(newNode);
      }
    }
    
    sg::DataBuffer *parseDataNode(xml::Node *node)
    {
#if 1
      NOTIMPLEMENTED;
#else
      assert(node->name == "Data");
      Data *data = new Data;
      assert(node->child.empty());
      size_t num = atol(node->getProp("num").c_str());
      size_t ofs = atol(node->getProp("ofs").c_str());
      return data;
#endif
    }

    sg::Node *parseNode(xml::Node *node)
    {
      if (node->name == "Data")
        return parseDataNode(node);
      if (node->name == "Info")
        return parseInfoNode(node);
      if (node->name == "Integrator")
        return parseIntegratorNode(node);
      std::cout << "warning: unknown sg::Node type '" << node->name << "'" << std::endl;
      return NULL;
    }

    Ref<sg::World> loadOSP(const std::string &fileName)
    {
      xml::XMLDoc *doc = NULL;
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

      xml::Node *root = doc->child[0];
      Ref<sg::World> world = new World;//parseOSPRaySection(root->child[0]); 
      if (root->child.size() == 1 && root->child[0]->name == "World") {
        parseWorldNode(world.ptr,root->child[0],binBasePtr);
      } else {
        parseWorldNode(world.ptr,root,binBasePtr);
      }
      
      cout << "#osp:sg: done parsing OSP file" << endl;
      return world;
    }

  } // ::ospray::sg
} // ::ospray

