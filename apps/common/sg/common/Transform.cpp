// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
      createChild("visible", "bool", true);
      createChild("position", "vec3f");
      createChild("rotation", "vec3f", vec3f(0),
                  NodeFlags::required |
                  NodeFlags::gui_slider).setMinMax(-vec3f(2*3.15f),
                                                    vec3f(2*3.15f));
      createChild("rotationOrder", "string", std::string("zyx"),
                  NodeFlags::required |
                  NodeFlags::valid_whitelist |
                  NodeFlags::gui_combo)
        .setWhiteList({std::string("xyz"), std::string("xzy"), std::string("yxz"),
                       std::string("yzx"), std::string("zxy"), std::string("zyx")});
      createChild("scale", "vec3f", vec3f(1.f));
      createChild("userTransform", "affine3f");
    }


    std::string Transform::toString() const
    {
      return "ospray::sg::Transform";
    }

    void Transform::preCommit(RenderContext &ctx)
    {
      updateTransform(ctx.currentTransform);
      preRender(ctx);
    }

    void Transform::postCommit(RenderContext& ctx)
    {
      postRender(ctx);
      Renderable::postCommit(ctx);
    }

    void Transform::preRender(RenderContext &ctx)
    {
      if (ctx.currentTransform != cachedTransform)
        updateTransform(ctx.currentTransform);
      ctx.currentTransform = worldTransform;
    }

    void Transform::postRender(RenderContext &ctx)
    {
      ctx.currentTransform = cachedTransform;
    }

    void Transform::updateTransform(const ospcommon::affine3f& transform)
    {
      const vec3f scale = child("scale").valueAs<vec3f>();
      const vec3f rotation = child("rotation").valueAs<vec3f>();
      const std::string rotationOrder = child("rotationOrder").valueAs<std::string>();
      const vec3f translation = child("position").valueAs<vec3f>();
      const ospcommon::affine3f userTransform = child("userTransform").valueAs<affine3f>();

      ospcommon::affine3f rotationTransform {one};
      for (char axis : rotationOrder) {
        switch (tolower(axis)) {
        default:
        case 'x': rotationTransform = ospcommon::affine3f::rotate(vec3f(1,0,0),rotation.x) * rotationTransform; break;
        case 'y': rotationTransform = ospcommon::affine3f::rotate(vec3f(0,1,0),rotation.y) * rotationTransform; break;
        case 'z': rotationTransform = ospcommon::affine3f::rotate(vec3f(0,0,1),rotation.z) * rotationTransform; break;
        }
      }
      worldTransform = transform*userTransform*ospcommon::affine3f::translate(translation)*
        rotationTransform*ospcommon::affine3f::scale(scale);
      cachedTransform = transform;
    }

    //OSPRay common
    OSP_REGISTER_SG_NODE_NAME(Affine3f, affine3f);
    OSP_REGISTER_SG_NODE(Transform);

  } // ::ospray::sg
} // ::ospray

