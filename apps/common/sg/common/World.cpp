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

    Model::Model()
    {
      setValue(ospNewModel());
    }

    std::string Model::toString() const
    {
      return "ospray::sg::Model";
    }

    void Model::traverse(RenderContext &ctx, const std::string& operation)
    {
      if (operation == "render") {
        preRender(ctx);
        postRender(ctx);
      }
      else
        Node::traverse(ctx,operation);
    }

    void Model::preCommit(RenderContext &ctx)
    {
      auto model = ospModel();
      if (model)
        ospRelease(model);
      model = ospNewModel();
      setValue(model);
      stashedModel = ctx.currentOSPModel;
      ctx.currentOSPModel = model;
    }

    void Model::postCommit(RenderContext &ctx)
    {
      auto model = ospModel();
      ctx.currentOSPModel = model;

      //instancegroup caches render calls in commit.
      for (auto child : properties.children)
        child.second->traverse(ctx, "render");

      ospCommit(model);
      ctx.currentOSPModel = stashedModel;
      child("bounds") = computeBounds();
    }

    OSPModel Model::ospModel()
    {
      return (OSPModel)valueAs<OSPObject>();
    }

    std::string World::toString() const
    {
      return "ospray::sg::World";
    }

    void World::preCommit(RenderContext &ctx)
    {
      stashedWorld = ctx.world;
      ctx.world = this->nodeAs<sg::World>();
      Model::preCommit(ctx);
    }

    void World::postCommit(RenderContext &ctx)
    {
      Model::postCommit(ctx);
      ctx.world = stashedWorld;
    }

    Instance::Instance()
      : World()
    {
      createChild("visible", "bool", true);
      createChild("position", "vec3f");
      createChild("rotation", "vec3f", vec3f(0),
                  NodeFlags::required      |
                  NodeFlags::valid_min_max |
                  NodeFlags::gui_slider).setMinMax(-vec3f(2*3.15f),
                                                    vec3f(2*3.15f));
      createChild("scale", "vec3f", vec3f(1.f));
      createChild("model", "Model");
    }

        /*! \brief return bounding box in world coordinates.

      This function can be used by the viewer(s) for calibrating
      camera motion, setting default camera position, etc. Nodes
      for which that does not apply can simpy return
      box3f(empty) */
    box3f Instance::bounds() const
    {
      box3f cbounds = child("model").bounds();
      if (cbounds.empty())
        return cbounds;
      const vec3f lo = cbounds.lower;
      const vec3f hi = cbounds.upper;
      box3f bounds = ospcommon::empty;
      bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,lo.y,lo.z)));
      bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,lo.y,lo.z)));
      bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,hi.y,lo.z)));
      bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,hi.y,lo.z)));
      bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,lo.y,hi.z)));
      bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,lo.y,hi.z)));
      bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,hi.y,hi.z)));
      bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,hi.y,hi.z)));
      return bounds;
    }

    void Instance::traverse(RenderContext &ctx, const std::string& operation)
    {
      if (instanced && operation == "render") {
        preRender(ctx);
        postRender(ctx);
      }
      else
        Node::traverse(ctx,operation);
    }

    void Instance::preCommit(RenderContext &ctx)
    {
      if (instanced) {
        instanceDirty=true;

        oldTransform = ctx.currentTransform;

        updateTransform(ctx);
        cachedTransform=ctx.currentTransform;
        ctx.currentTransform = worldTransform;
      }
    }

    void Instance::postCommit(RenderContext &ctx)
    {
      if (instanced)
        ctx.currentTransform = oldTransform;
      child("bounds").setValue(computeBounds());
    }

    void Instance::preRender(RenderContext &ctx)
    {
      if (instanced) {
        oldTransform = ctx.currentTransform;
        if (cachedTransform != ctx.currentTransform)
          instanceDirty=true;
        if (instanceDirty)
          updateInstance(ctx);
        ctx.currentTransform = worldTransform;
      }
    }

    void Instance::postRender(RenderContext &ctx)
    {
      if (instanced && child("visible").value() == true
        && ctx.world && ctx.world->ospModel() && ospInstance)
      {
        ospAddGeometry(ctx.world->ospModel(), ospInstance);
      }
      ctx.currentTransform = oldTransform;
    }

    void Instance::updateTransform(RenderContext &ctx)
    {
      vec3f scale = child("scale").valueAs<vec3f>();
      vec3f rotation = child("rotation").valueAs<vec3f>();
      vec3f translation = child("position").valueAs<vec3f>();
      worldTransform = ctx.currentTransform*baseTransform*ospcommon::affine3f::translate(translation)*
      ospcommon::affine3f::rotate(vec3f(1,0,0),rotation.x)*
      ospcommon::affine3f::rotate(vec3f(0,1,0),rotation.y)*
      ospcommon::affine3f::rotate(vec3f(0,0,1),rotation.z)*
      ospcommon::affine3f::scale(scale);
    }

    void Instance::updateInstance(RenderContext &ctx)
    {
      updateTransform(ctx);
      cachedTransform=ctx.currentTransform;

      if (ospInstance)
        ospRelease(ospInstance);
      ospInstance = nullptr;

      auto model = child("model").valueAs<OSPModel>();
      if (model) {
        ospInstance = ospNewInstance(model,(osp::affine3f&)worldTransform);
        ospCommit(ospInstance);
      }
      instanceDirty=false;
    }

    OSP_REGISTER_SG_NODE(Model);
    OSP_REGISTER_SG_NODE(World);
    OSP_REGISTER_SG_NODE(Instance);

  } // ::ospray::sg
} // ::ospray
