/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG
#include "SceneGraph.h"
// xml
#include "apps/common/xml/xml.h"
#include "ospray/common/Library.h"
// embree stuff
#include "common/sys/library.h"

namespace ospray {
  namespace sg {

    // ==================================================================
    // parameter type specializations
    // ==================================================================
    template<> OSPDataType ParamT<Ref<Data> >::getOSPDataType() const
    { return OSP_DATA; }
    template<> OSPDataType ParamT<Ref<Node> >::getOSPDataType() const
    { return OSP_OBJECT; }

    template<> OSPDataType ParamT<float>::getOSPDataType() const
    { return OSP_FLOAT; }
    template<> OSPDataType ParamT<vec2f>::getOSPDataType() const
    { return OSP_FLOAT2; }
    template<> OSPDataType ParamT<vec3f>::getOSPDataType() const
    { return OSP_FLOAT3; }
    template<> OSPDataType ParamT<vec4f>::getOSPDataType() const
    { return OSP_FLOAT4; } 

    template<> OSPDataType ParamT<int32>::getOSPDataType() const
    { return OSP_INT; }
    template<> OSPDataType ParamT<vec2i>::getOSPDataType() const
    { return OSP_INT2; }
    template<> OSPDataType ParamT<vec3i>::getOSPDataType() const
    { return OSP_INT3; }
    template<> OSPDataType ParamT<vec4i>::getOSPDataType() const
    { return OSP_INT4; }


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

    World *readXML(const std::string &fileName)
    {
      // World *world = new World;
      Ref<xml::XMLDoc> doc = NULL;
      doc = xml::readXML(fileName);
      if (!doc) 
        throw std::runtime_error("could not parse "+fileName);
      if (doc->child.size() != 1 || doc->child[0]->name != "ospray") 
        throw std::runtime_error("not an ospray xml file");
      World *world = parseWorldNode(doc->child[0]);
      return world;
    }


    void Serialization::serialize(Ref<sg::World> world, Serialization::Mode mode)
    {
      clear(); 
      Serialization::State state;
      state.serialization = this;
      world.ptr->serialize(state);
    }

    //! serialize into given serialization state 
    void sg::World::serialize(sg::Serialization::State &state)
    {
      sg::Serialization::State savedState = state;
      {
        //state->serialization->object.push_back(Serialization::);
        for (size_t i=0; i<node.size(); i++)
          node[i]->serialize(state);
      }
      state = savedState;
    }

    void Node::serialize(sg::Serialization::State &state)
    { 
      state.serialization->object.push_back(new Serialization::Object(this,state.instantiation.ptr));
    }

    void Integrator::commit()
    {
      if (!ospRenderer) {
        ospRenderer = ospNewRenderer(type.c_str());
        if (!ospRenderer) 
          throw std::runtime_error("#ospQTV: could not create renderer");
      }
      PING;
      if (lastCommitted >= lastModified) return;
      PING;

      // set world, camera, ...
      if (world ) { 
        world->commit();
        ospSetObject(ospRenderer,"world", world->ospModel);
      }
      PING;
      if (camera) {
        camera->commit();
        ospSetObject(ospRenderer,"camera",camera->ospCamera);
      }
      PING;

      lastCommitted = __rdtsc();
      PING;
      ospCommit(ospRenderer);

      PING;
      assert(ospRenderer); 
   }

    void Node::render(World *world, 
                       Integrator *integrator,
                       const affine3f &_xfm)
    {
      NOTIMPLEMENTED;
    }

    void Spheres::render(World *world, 
                       Integrator *integrator,
                       const affine3f &_xfm)
    {
      assert(!ospGeometry);

      ospGeometry = ospNewGeometry("spheres");
      assert(ospGeometry);

      OSPData data = ospNewData(sphere.size()*5,OSP_FLOAT,
                                &sphere[0],OSP_DATA_SHARED_BUFFER);
      ospSetData(ospGeometry,"spheres",data);

      // ospSet1f(geom,"radius",radius*particleModel[i]->radius);
      ospSet1i(ospGeometry,"bytes_per_sphere",sizeof(Spheres::Sphere));
      ospSet1i(ospGeometry,"center_offset",     0*sizeof(float));
      ospSet1i(ospGeometry,"offset_radius",     3*sizeof(float));
      ospSet1i(ospGeometry,"offset_materialID", 4*sizeof(float));
      // ospSetData(geom,"materialList",materialData);

      OSPMaterial mat = ospNewMaterial(integrator?integrator->ospRenderer:NULL,
                                       "default");
      if (mat) {
        vec3f kd = .7f;
        ospSet3fv(mat,"kd",&kd.x);
      }
      ospSetMaterial(ospGeometry,mat);
      ospCommit(ospGeometry);
      
      ospAddGeometry(world->ospModel,ospGeometry);
      ospCommit(data);

      
    }

    void World::render(World *world, 
                       Integrator *integrator,
                       const affine3f &_xfm)
    {
      PING;
      if (ospModel)
        throw std::runtime_error("World::ospModel alrady exists!?");
      ospModel = ospNewModel();
      affine3f xfm = embree::one;
      for (size_t i=0;i<node.size();i++)
        node[i]->render(this,integrator,xfm);
      ospCommit(ospModel);
    }

    void PerspectiveCamera::commit() 
    {
      if (!ospCamera) create(); 
      
      // const vec3f from = frame->sourcePoint;
      // const vec3f at   = frame->targetPoint;

      ospSetVec3f(ospCamera,"pos",from);
      ospSetVec3f(ospCamera,"dir",at - from);
      ospSetVec3f(ospCamera,"up",up);
      ospSetf(ospCamera,"aspect",aspect); //size.x/float(size.y));
      ospSetf(ospCamera,"fovy",fovy); //size.x/float(size.y));
      ospCommit(ospCamera);      
    }
    
  } // ::ospray::sg
} // ::ospray
