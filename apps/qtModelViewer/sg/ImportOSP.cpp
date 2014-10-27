/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG

// O_LARGEFILE is a GNU extension.
#ifdef __APPLE__
#define  O_LARGEFILE  0
#endif

// header
#include "SceneGraph.h"
// stl
#include <map>
// // libxml
#include "apps/common/xml/xml.h"
// stdlib, for mmap
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
// xml
#include "apps/common/xml/xml.h"
#include "ospray/common/Library.h"
// embree stuff
#include "common/sys/library.h"

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
    sg::Node *createNodeFrom(const xml::Node *node)
    {
      std::map<std::string, creatorFct>::iterator it = sgNodeRegistry.find(node->name);
      creatorFct creator = NULL;
      if (it == sgNodeRegistry.end()) {
        std::string creatorName = "ospray_create_sg_node__"+std::string(node->name);
        creator = (creatorFct)getSymbol(creatorName);
        if (!creator)
          throw std::runtime_error("unknown ospray scene graph node '"+node->name+"'");
        sgNodeRegistry[node->name] = creator;
      } else creator = it->second;
      assert(creator);
      sg::Node *newNode = creator();
      assert(newNode);
      try {
        newNode->setFrom(node);
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
        sg::Data *dataNode = dynamic_cast<sg::Data *>(value);
        assert(dataNode);
        target->addParam(new ParamT<Ref<Data> >(name,dataNode));
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
        target->addParam(new ParamT<int32>(name,atoi(node->content.c_str())));
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
    
    sg::World *parseWorldNode(xml::Node *node)
    {
      assert(node->name == "World");
      World *world = new World;
      for (int i=0;i<node->child.size();i++) {
        xml::Node *c = node->child[i];
        if (c->name == "Geometry") {
          //world->geometry.push_back(parseGeometryNode(c));
          throw std::runtime_error("unspecified geometry node!?");
        } else 
          throw std::runtime_error("unknown node type '"+c->name
                                   +"' in ospray::sg::World node");
      }
      return world;
    }
    
    sg::Data *parseDataNode(xml::Node *node)
    {
      assert(node->name == "Data");
      Data *data = new Data;
      assert(node->child.empty());
      size_t num = atol(node->getProp("num").c_str());
      size_t ofs = atol(node->getProp("ofs").c_str());
      return data;
    }
    
    //     /*! parse a 'ospray' node in the xml file */
    //     void parseWorld(World *world, xml::Node *root)
    //     {
    // #if 0
    //       for (int cID=0;cID<root->child.size();cID++) {
    //         xml::Node *node = root->child[cID];
    //         if (node->name == "Info") {
    //           world->node.push_back(parseInfoNode(node));
    //         } else if (node->name == "Model") {
    //           world->node.push_back(parseModelNode(node));
    //         } else if (node->name == "Integrator") {
    //           world->integrator.push_back(parseIntegratorNode(node));
    //         } else {
    //           world->node.push_back(ospray::sg::createNodeFrom(node));
    //         }
    //       }
    // #endif
    //     }

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

    // World *readXML(const std::string &fileName)
    // {
    //   // World *world = new World;
    //   Ref<xml::XMLDoc> doc = NULL;
    //   doc = xml::readXML(fileName);
    //   if (!doc) 
    //     throw std::runtime_error("could not parse "+fileName);
    //   if (doc->child.size() != 1 || doc->child[0]->name != "ospray") 
    //     throw std::runtime_error("not an ospray xml file");
    //   World *world = parseWorldNode(doc->child[0]);
    //   return world;
    // }



    Ref<sg::World> loadOSP(const std::string &fileName)
    {
      Ref<xml::XMLDoc> doc = NULL;
      doc = xml::readXML(fileName);
      if (!doc) 
        throw std::runtime_error("could not parse "+fileName);
      if (doc->child.size() != 1 || doc->child[0]->name != "ospray") 
        throw std::runtime_error("not an ospray xml file");

      Ref<sg::World> world = parseWorldNode(doc->child[0]);
      return world;
    }

  } // ::ospray::sg
} // ::ospray

