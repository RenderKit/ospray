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
      : baseTransform(ospcommon::one), worldTransform(ospcommon::one)
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
    }


    std::string Transform::toString() const
    {
      return "ospray::sg::Transform";
    }
    
    /*! \brief return bounding box in world coordinates.
      
      This function can be used by the viewer(s) for calibrating
      camera motion, setting default camera position, etc. Nodes
      for which that does not apply can simpy return
      box3f(empty) */
    // box3f Transform::bounds() const
    // {
    //   assert(node);
    //   const box3f nodeBounds = node->bounds();
    //   const vec3f lo = nodeBounds.lower;
    //   const vec3f hi = nodeBounds.upper;
    //   box3f bounds = ospcommon::empty;
    //   bounds.extend(xfmPoint(xfm,vec3f(lo.x,lo.y,lo.z)));
    //   bounds.extend(xfmPoint(xfm,vec3f(hi.x,lo.y,lo.z)));
    //   bounds.extend(xfmPoint(xfm,vec3f(lo.x,hi.y,lo.z)));
    //   bounds.extend(xfmPoint(xfm,vec3f(hi.x,hi.y,lo.z)));
    //   bounds.extend(xfmPoint(xfm,vec3f(lo.x,lo.y,hi.z)));
    //   bounds.extend(xfmPoint(xfm,vec3f(hi.x,lo.y,hi.z)));
    //   bounds.extend(xfmPoint(xfm,vec3f(lo.x,hi.y,hi.z)));
    //   bounds.extend(xfmPoint(xfm,vec3f(hi.x,hi.y,hi.z)));
    //   return bounds;
    // }


        /*! \brief return bounding box in world coordinates.
      
      This function can be used by the viewer(s) for calibrating
      camera motion, setting default camera position, etc. Nodes
      for which that does not apply can simpy return
      box3f(empty) */
    // box3f Transform::bounds() const
    // {
    //   box3f cbounds = empty;
    //   for (const auto &child : properties.children)
    //     cbounds.extend(child.second->bounds());
    //   if (cbounds.empty())
    //     return cbounds;
    //   const vec3f lo = cbounds.lower;
    //   const vec3f hi = cbounds.upper;
    //   box3f bounds = ospcommon::empty;
    //   bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,lo.y,lo.z)));
    //   bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,lo.y,lo.z)));
    //   bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,hi.y,lo.z)));
    //   bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,hi.y,lo.z)));
    //   bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,lo.y,hi.z)));
    //   bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,lo.y,hi.z)));
    //   bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,hi.y,hi.z)));
    //   bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,hi.y,hi.z)));
    //   return bounds;
    // }

    void Transform::preRender(RenderContext &ctx)
    {
      vec3f scale = child("scale").valueAs<vec3f>();
      vec3f rotation = child("rotation").valueAs<vec3f>();
      vec3f translation = child("position").valueAs<vec3f>();
      worldTransform = ctx.currentTransform*baseTransform*ospcommon::affine3f::translate(translation)*
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

    OSP_REGISTER_SG_NODE(Transform);

  } // ::ospray::sg
} // ::ospray
  
