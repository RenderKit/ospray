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

#include "sg/camera/PerspectiveCamera.h"

namespace ospray {
  namespace sg {

    PerspectiveCamera::PerspectiveCamera() 
      : Camera("perspective"),
        from(0,-1,0), at(0,0,0), up(0,0,1), aspect(1),
        fovy(60)
    {
      create(); 
    }
    void PerspectiveCamera::init()
    {
      add(createNode("pos", "vec3f", from));
      add(createNode("dir", "vec3f", at,NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("dir").setMinMax(vec3f(-1), vec3f(1));
      add(createNode("up", "vec3f", up,NodeFlags::required));
      add(createNode("aspect", "float", aspect,NodeFlags::required | NodeFlags::valid_min_max));
      child("aspect").setMinMax(float(1e-31), float(1e31));
      add(createNode("fovy", "float", fovy,NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
      child("fovy").setMinMax(.1f, 360.f);
    }

    void PerspectiveCamera::commit() 
    {
      if (!ospCamera) create(); 
      
      ospSetVec3f(ospCamera,"pos",(const osp::vec3f&)from);
      vec3f dir = (at - from);
      ospSetVec3f(ospCamera,"dir",(const osp::vec3f&)dir);
      ospSetVec3f(ospCamera,"up",(const osp::vec3f&)up);
      ospSetf(ospCamera,"aspect",aspect);
      ospSetf(ospCamera,"fovy",fovy);
      ospCommit(ospCamera);      
    }

    void PerspectiveCamera::postCommit(RenderContext &ctx)
    {
      if (!ospCamera) create(); 
      
      ospSetVec3f(ospCamera,"pos",(const osp::vec3f&)child("pos").valueAs<vec3f>());
      ospSetVec3f(ospCamera,"dir",(const osp::vec3f&)child("dir").valueAs<vec3f>());
      ospSetVec3f(ospCamera,"up",(const osp::vec3f&)child("up").valueAs<vec3f>());
      ospSetf(ospCamera,"aspect",child("aspect").valueAs<float>());
      ospSetf(ospCamera,"fovy",child("fovy").valueAs<float>());
      ospCommit(ospCamera);  
    }

    OSP_REGISTER_SG_NODE(PerspectiveCamera);
    
  } // ::ospray::sg
} // ::ospray


