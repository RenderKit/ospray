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

#include "sg/camera/PanoramicCamera.h"

namespace ospray {
  namespace sg {

    PanoramicCamera::PanoramicCamera()
      : Camera("panoramic")
    {
      createChild("pos", "vec3f", vec3f(0, -1, 0));
      createChild("dir", "vec3f", vec3f(0, 0, 0),
                       NodeFlags::required | NodeFlags::valid_min_max |
                       NodeFlags::required | NodeFlags::valid_min_max |
                       NodeFlags::gui_slider).setMinMax(vec3f(-1), vec3f(1));
      createChild("up", "vec3f", vec3f(0, 0, 1),NodeFlags::required);
    }

    void PanoramicCamera::postCommit(RenderContext &ctx)
    {
      if (!ospCamera) create();

      ospSetVec3f(ospCamera,"pos",(const osp::vec3f&)child("pos").valueAs<vec3f>());
      ospSetVec3f(ospCamera,"dir",(const osp::vec3f&)child("dir").valueAs<vec3f>());
      ospSetVec3f(ospCamera,"up",(const osp::vec3f&)child("up").valueAs<vec3f>());
      ospCommit(ospCamera);
    }

    OSP_REGISTER_SG_NODE(PanoramicCamera);

  } // ::ospray::sg
} // ::ospray

