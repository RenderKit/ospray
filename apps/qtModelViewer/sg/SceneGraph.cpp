/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG
#include "SceneGraph.h"

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
    
  } // ::ospray::sg
} // ::ospray
