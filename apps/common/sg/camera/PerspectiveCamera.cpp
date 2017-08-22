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
      : Camera("perspective")
    {
      createChild("pos", "vec3f", vec3f(0, -1, 0));
      // XXX SG is too restrictive: OSPRay cameras accept non-normalized directions
      createChild("dir", "vec3f", vec3f(0, 0, 0),
                       NodeFlags::required | NodeFlags::valid_min_max |
                       NodeFlags::gui_slider).setMinMax(vec3f(-1), vec3f(1));
      createChild("up", "vec3f", vec3f(0, 0, 1),NodeFlags::required);
      createChild("aspect", "float", 1.f,
                      NodeFlags::required |
                      NodeFlags::valid_min_max).setMinMax(1e-31f, 1e31f);
      createChild("fovy", "float", 60.f,
                      NodeFlags::required | NodeFlags::valid_min_max |
                      NodeFlags::gui_slider).setMinMax(.1f, 360.f);
      createChild("apertureRadius", "float", 0.f,
                      NodeFlags::valid_min_max).setMinMax(0.f, 1e31f);
      createChild("focusDistance", "float", 1.f,
                      NodeFlags::valid_min_max).setMinMax(0.f, 1e31f);
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


