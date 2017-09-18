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

// sg components
#include "Transform.h"

namespace ospray {
  namespace sg {


    Transform::Transform()
      : worldTransform(ospcommon::one)
    {
      createChild("bounds", "box3f");
      createChild("visible", "bool", true);
      createChild("position", "vec3f");
      createChild("rotation", "vec3f", vec3f(0),
                     NodeFlags::required      |
                     NodeFlags::valid_min_max |
                     NodeFlags::gui_slider).setMinMax(-vec3f(2*3.15f),
                                                      vec3f(2*3.15f));
      createChild("scale", "vec3f", vec3f(1.f));
      createChild("userTransform", "affine3f");
    }


    std::string Transform::toString() const
    {
      return "ospray::sg::Transform";
    }

    void Transform::preRender(RenderContext &ctx)
    {
      const vec3f scale = child("scale").valueAs<vec3f>();
      const vec3f rotation = child("rotation").valueAs<vec3f>();
      const vec3f translation = child("position").valueAs<vec3f>();
      const ospcommon::affine3f userTransform = child("userTransform").valueAs<affine3f>();
      worldTransform = ctx.currentTransform*userTransform*ospcommon::affine3f::translate(translation)*
      ospcommon::affine3f::rotate(vec3f(1,0,0),rotation.x)*
      ospcommon::affine3f::rotate(vec3f(0,1,0),rotation.y)*
      ospcommon::affine3f::rotate(vec3f(0,0,1),rotation.z)*
      ospcommon::affine3f::scale(scale);

      oldTransform = ctx.currentTransform;
      ctx.currentTransform = worldTransform;
    }

    void Transform::postRender(RenderContext &ctx)
    {
      ctx.currentTransform = oldTransform;
    }

    //OSPRay common
    OSP_REGISTER_SG_NODE_NAME(Affine3f, affine3f);
    OSP_REGISTER_SG_NODE(Transform);

  } // ::ospray::sg
} // ::ospray
  
