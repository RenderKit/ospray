/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

/*! \file sg.cpp Scene Graph for OSPRay model viewer (Implementation) */

#undef NDEBUG
#include "sg.h"
#include "apps/common/xml/xml.h"
#include "../common/Library.h"

namespace ospray {
  namespace sg {

    // ==================================================================
    // parameter type specializations
    // ==================================================================
    template<> OSPDataType ParamT<Ref<Data> >::getOSPDataType() const
    { return OSP_DATA; }
    template<> OSPDataType ParamT<Ref<Node> >::getOSPDataType() const
    { return OSP_OBJECT; }
    template<> OSPDataType ParamT<int32>::getOSPDataType() const
    { return OSP_INT; }
    template<> OSPDataType ParamT<float>::getOSPDataType() const
    { return OSP_FLOAT; }

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
    // sg node implementations
    // ==================================================================
    void sg::Node::addParam(sg::Param *p)
    {
      assert(p);
      assert(p->getName() != "");
      param[p->getName()] = p;
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
    
    sg::Geometry *parseGeometryNode(xml::Node *node)
    {
      assert(node->name == "Geometry");
      Geometry *geometry = new Geometry(node->getProp("type"));
      for (int i=0;i<node->child.size();i++) {
        xml::Node *c = node->child[i];
        if (parseParam(geometry,c))
          continue;
        throw std::runtime_error("unknown node type '"+c->name
                                 +"' in ospray::sg::Geometry node");
      }
      return geometry;
    }
    
    sg::Renderer *parseRendererNode(xml::Node *node)
    {
      assert(node->name == "Renderer");
      Renderer *renderer = new Renderer(node->getProp("type"));
      for (int i=0;i<node->child.size();i++) {
        xml::Node *c = node->child[i];
        if (parseParam(renderer,c))
          continue;
        throw std::runtime_error("unknown node type '"+c->name
                                 +"' in ospray::sg::Renderer node");
      }
      return renderer;
    }
    
    sg::Model *parseModelNode(xml::Node *node)
    {
      assert(node->name == "Model");
      Model *model = new Model;
      for (int i=0;i<node->child.size();i++) {
        xml::Node *c = node->child[i];
        if (c->name == "Geometry")
          model->geometry.push_back(parseGeometryNode(c));
        else 
          throw std::runtime_error("unknown node type '"+c->name
                                   +"' in ospray::sg::Model node");
      }
      return model;
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
    
    /*! parse a 'ospray' node in the xml file */
    void parseWorld(World *world, xml::Node *root)
    {
      for (int cID=0;cID<root->child.size();cID++) {
        xml::Node *node = root->child[cID];
        if (node->name == "Info") {
          world->node.push_back(parseInfoNode(node));
        } else if (node->name == "Model") {
          world->node.push_back(parseModelNode(node));
        } else if (node->name == "Renderer") {
          world->node.push_back(parseRendererNode(node));
        } else {
          world->node.push_back(ospray::sg::createNodeFrom(node));
        }
      }
    }

    sg::Node *parseNode(xml::Node *node)
    {
      if (node->name == "Data")
        return parseDataNode(node);
      if (node->name == "Info")
        return parseInfoNode(node);
      if (node->name == "Renderer")
        return parseRendererNode(node);
      std::cout << "warning: unknown sg::Node type '" << node->name << "'" << std::endl;
      return NULL;
    }

    World *readXML(const std::string &fileName)
    {
      World *world = new World;
      xml::XMLDoc *doc = NULL;
      try {
        doc = xml::readXML(fileName);
        if (!doc) 
          throw std::runtime_error("could not parse "+fileName);
        if (doc->child.size() != 1 || doc->child[0]->name != "ospray") 
          throw std::runtime_error("not an ospray xml file");
        parseWorld(world,doc->child[0]);
        return world;
      } catch (std::runtime_error e) {
        delete world;
        throw e;
      }
    }

  };
}
