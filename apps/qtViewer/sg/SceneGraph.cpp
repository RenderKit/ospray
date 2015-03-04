// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

// scene graph
#include "SceneGraph.h"
#include "sg/common/Texture2D.h"
#include "sg/geometry/Spheres.h"

namespace ospray {
  namespace sg {

    // ==================================================================
    // parameter type specializations
    // ==================================================================
    template<> OSPDataType ParamT<Ref<DataBuffer> >::getOSPDataType() const
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

    template<> OSPDataType ParamT<const char *>::getOSPDataType() const
    { return OSP_STRING; }
    template<> OSPDataType ParamT<Ref<Texture2D> >::getOSPDataType() const
    { return OSP_OBJECT; }

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

    void Node::serialize(sg::Serialization::State &state)
    { 
      state.serialization->object.push_back(new Serialization::Object(this,state.instantiation.ptr));
    }

<<<<<<< HEAD
=======
    void Integrator::commit()
    {
      if (!ospRenderer) {
        ospRenderer = ospNewRenderer(type.c_str());
        if (!ospRenderer) 
          throw std::runtime_error("#osp:sg:SceneGraph: could not create renderer (of type '"+type+"')");
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

      ospSet1i(ospRenderer,"spp",spp);

      lastCommitted = __rdtsc();
      ospCommit(ospRenderer);
      assert(ospRenderer); 
   }

>>>>>>> 4814bd9beb6bcd8df0d16f2c4e0ff0a57f463a3d
  } // ::ospray::sg
} // ::ospray
