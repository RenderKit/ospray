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

#include "sg/common/World.h"

namespace ospray {
  namespace sg {

    box3f World::bounds() const
    {
      box3f bounds = empty;
      for (const auto &child : properties.children)
        bounds.extend(child.second->bounds());
      return bounds;
    }

    //! serialize into given serialization state
    void sg::World::serialize(sg::Serialization::State &state)
    {
      sg::Serialization::State savedState = state;
      for (auto node: nodes)
        node->serialize(state);
      state = savedState;
    }

    void World::render(RenderContext &ctx)
    {
    }

    void World::preCommit(RenderContext &ctx)
    {
      oldWorld = ctx.world;
      ctx.world = std::static_pointer_cast<sg::World>(shared_from_this());
      ospModel = ospNewModel();
      ospCommit(ospModel);
      setValue((OSPObject)ospModel);
    }

    void World::postCommit(RenderContext &ctx)
    {
      ospCommit(ospModel);
      ctx.world = oldWorld;
    }

    void World::preRender(RenderContext &ctx)
    {
      preCommit(ctx);
    }

    void World::postRender(RenderContext &ctx)
    {
       ospCommit(ospModel);
       ctx.world = oldWorld;
    }

    void InstanceGroup::preCommit(RenderContext &ctx)
    {
      if (instanced) {
        oldWorld = ctx.world;
        ctx.world = std::static_pointer_cast<sg::World>(shared_from_this());
        if (!ospModel)  //TODO: add support for changing initial geometry
        {
          ospModel = ospNewModel();
          ospCommit(ospModel);
          setValue((OSPObject)ospModel);
        }
      }
    }

    void InstanceGroup::postCommit(RenderContext &ctx)
    {
      if (instanced) {
        ospCommit(ospModel);
        ctx.world = oldWorld;
      }
    }

    void InstanceGroup::preRender(RenderContext &ctx)
    {
      oldWorld = ctx.world;
      if (instanced) {
        ctx.world = std::static_pointer_cast<sg::World>(shared_from_this());
        vec3f scale = child("scale")->valueAs<vec3f>();
        vec3f rotation = child("rotation")->valueAs<vec3f>();
        vec3f translation = child("position")->valueAs<vec3f>();
        ospcommon::affine3f xfm = ospcommon::one;
        xfm = xfm*ospcommon::affine3f::translate(translation)*ospcommon::affine3f::rotate(vec3f(1,0,0),rotation.x)*
        ospcommon::affine3f::rotate(vec3f(0,1,0),rotation.y)*
        ospcommon::affine3f::rotate(vec3f(0,0,1),rotation.z)*
        ospcommon::affine3f::scale(scale);
        
        ospInstance = ospNewInstance(ospModel,(osp::affine3f&)xfm);
        ospCommit(ospInstance);

        if (child("visible")->value() == true)
          ospAddGeometry(oldWorld->ospModel,ospInstance);
      }
    }

    void InstanceGroup::postRender(RenderContext &ctx)
    {
      ctx.world = oldWorld;
    }

    OSP_REGISTER_SG_NODE(World);
    OSP_REGISTER_SG_NODE(InstanceGroup);
  }
}
