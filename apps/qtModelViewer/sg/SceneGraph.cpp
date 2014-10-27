/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG
#include "SceneGraph.h"
#include "apps/common/xml/xml.h"

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
    // sg node implementations
    // ==================================================================
    void sg::Node::addParam(sg::Param *p)
    {
      assert(p);
      assert(p->getName() != "");
      param[p->getName()] = p;
    }

    // list of all named nodes
    std::map<std::string,Ref<sg::Node> > namedNodes;
    sg::Node *findNamedNode(const std::string &name) { if (namedNodes.find(name) != namedNodes.end()) return namedNodes[name].ptr; return NULL; }
    void registerNamedNode(const std::string &name, Ref<sg::Node> node) { namedNodes[name] = node; }
    


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
      if (lastCommitted >= lastModified) return;

      // set world, camera, ...
      if (world ) { 
        world->commit();
        ospSetObject(ospRenderer,"world", world->ospModel);
      }
      if (camera) {
        camera->commit();
        ospSetObject(ospRenderer,"camera",camera->ospCamera);
      }

      lastCommitted = __rdtsc();
      ospCommit(ospRenderer);
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

    void TransferFunction::render(World *world, 
                                  Integrator *integrator,
                                  const affine3f &xfm)
    {
      if (!ospTransferFunction) {
        ospTransferFunction = ospNewTransferFunction("piecewise_linear");
      }
      ospColorData = ospNewData(colorArray.size(),OSP_FLOAT3,&colorArray[0]); ospCommit(ospColorData);
      ospAlphaData = ospNewData(alphaArray.size(),OSP_FLOAT3,&alphaArray[0]); ospCommit(ospAlphaData);

      ospSetData(ospTransferFunction,"colors",ospColorData);
      ospSetData(ospTransferFunction,"alphas",ospAlphaData);
      ospCommit(ospTransferFunction);
    }

    void AlphaMappedSpheres::render(World *world, 
                                  Integrator *integrator,
                                  const affine3f &xfm)
    {
      assert(!ospGeometry);

      // JUST FOR TESTING: create a sphere right here ....
      sphere.push_back(Sphere(vec3f(0),1,1));

      ospGeometry = ospNewGeometry("spheres");
      assert(ospGeometry);


      OSPData data = ospNewData(sphere.size()*5,OSP_FLOAT,&sphere[0]);
      ospSetData(ospGeometry,"spheres",data);

      ospSet1i(ospGeometry,"bytes_per_sphere",sizeof(Spheres::Sphere));
      ospSet1i(ospGeometry,"center_offset",     0*sizeof(float));
      ospSet1i(ospGeometry,"offset_radius",     3*sizeof(float));
      ospSet1i(ospGeometry,"offset_materialID", 4*sizeof(float));

      assert(transferFunction);
      if (transferFunction) {
        transferFunction->render();
        assert(transferFunction->ospTransferFunction);
        ospSetObject(ospGeometry,"transferFunction",transferFunction->ospTransferFunction);
      }

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

    //! \brief Initialize this node's value from given corresponding XML node 
    void AlphaMappedSpheres::setFromXML(const xml::Node *const node)
    {
      PING;
      for (size_t childID=0;childID<node->child.size();childID++) {
        xml::Node *child = node->child[childID];
        if (child->name == "transferFunction") {
          if (child->getProp("ref") != "")
            transferFunction = dynamic_cast<sg::TransferFunction*>(findNamedNode(child->getProp("ref")));
          else if (child->child.size()) {
            Ref<sg::Node> n = sg::parseNode(child->child[0]);
            transferFunction = n.cast<sg::TransferFunction>();
          }
        }
        else
          std::cout << "#osp:sg:AlphaMappedSpheres: Warning - unknown child field type '" << child->name << "'" << std::endl;
      }

      if (!transferFunction) {
        std::cout << "#osp:sg:AlphaMappedSpheres: Warning - no transfer function specified" << std::endl;
        transferFunction = new TransferFunction();
      }
    }

    //! \brief Initialize this node's value from given corresponding XML node 
    void TransferFunction::setDefaultValues()
    {
      colorArray.clear();
      colorArray.push_back(osp::vec3f(0         , 0           , 0.562493   ));
      colorArray.push_back(osp::vec3f(0         , 0           , 1          ));
      colorArray.push_back(osp::vec3f(0         , 1           , 1          ));
      colorArray.push_back(osp::vec3f(0.500008  , 1           , 0.500008   ));
      colorArray.push_back(osp::vec3f(1         , 1           , 0          ));
      colorArray.push_back(osp::vec3f(1         , 0           , 0          ));
      colorArray.push_back(osp::vec3f(0.500008  , 0           , 0          ));

      alphaArray.clear();
      for (int i=0;i<colorArray.size();i++)
        alphaArray.push_back(i/float(colorArray.size()-1));
    }

    //! \brief Initialize this node's value from given corresponding XML node 
    void TransferFunction::setFromXML(const xml::Node *const node)
    {
    }

    void World::render(World *world, 
                       Integrator *integrator,
                       const affine3f &_xfm)
    {
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

    OSP_REGISTER_SG_NODE(TransferFunction)
    OSP_REGISTER_SG_NODE(AlphaMappedSpheres)
  } // ::ospray::sg
} // ::ospray
