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

#include "Instance.h"
#include "Model.h"

namespace ospray {
  namespace sg {

    Instance::Instance()
      : Renderable()
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
      createChild("model", "Model");
    }

        /*! \brief return bounding box in world coordinates.

      This function can be used by the viewer(s) for calibrating
      camera motion, setting default camera position, etc. Nodes
      for which that does not apply can simpy return
      box3f(empty) */
    box3f Instance::computeBounds() const
    {
      box3f cbounds = child("model").nodeAs<Model>()->computeBounds();
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
      Renderable::postCommit(ctx);
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
        && ctx.world && ctx.world->valueAs<OSPModel>() && ospInstance)
      {
        ospAddGeometry(ctx.world->valueAs<OSPModel>(), ospInstance);
      }
      ctx.currentTransform = oldTransform;
    }

    void Instance::updateTransform(RenderContext &ctx)
    {
      const vec3f scale = child("scale").valueAs<vec3f>();
      const vec3f rotation = child("rotation").valueAs<vec3f>();
      const std::string rotationOrder = child("rotationOrder").valueAs<std::string>();
      const vec3f translation = child("position").valueAs<vec3f>();

      ospcommon::affine3f rotationTransform {one};
      for (char axis : rotationOrder) {
        switch (tolower(axis)) {
        default:
        case 'x': rotationTransform = ospcommon::affine3f::rotate(vec3f(1,0,0),rotation.x) * rotationTransform; break;
        case 'y': rotationTransform = ospcommon::affine3f::rotate(vec3f(0,1,0),rotation.y) * rotationTransform; break;
        case 'z': rotationTransform = ospcommon::affine3f::rotate(vec3f(0,0,1),rotation.z) * rotationTransform; break;
        }
      }
      worldTransform = ctx.currentTransform*baseTransform*ospcommon::affine3f::translate(translation)*
        rotationTransform*ospcommon::affine3f::scale(scale);
    }

    void Instance::updateInstance(RenderContext &ctx)
    {
      updateTransform(ctx);
      cachedTransform=ctx.currentTransform;

      ospRelease(ospInstance);
      ospInstance = nullptr;

      auto model = child("model").valueAs<OSPModel>();
      if (model) {
        ospInstance = ospNewInstance(model,(osp::affine3f&)worldTransform);
        ospCommit(ospInstance);
      }
      instanceDirty=false;
    }

    InstanceGroup::InstanceGroup()
      : Renderable()
    {
      createChild("visible", "bool", true);
    }

        /*! \brief return bounding box in world coordinates.

      This function can be used by the viewer(s) for calibrating
      camera motion, setting default camera position, etc. Nodes
      for which that does not apply can simpy return
      box3f(empty) */
    box3f InstanceGroup::computeBounds() const
    {
      box3f bounds = ospcommon::empty;
      if (!(hasChild("models") && hasChild("transforms") && hasChild("indices")))
        return bounds;
      auto& indices = child("indices").nodeAs<DataVector2i>()->v;
      auto models = child("models").nodeAs<ModelList>();
      auto& transforms = child("transforms").nodeAs<DataVectorAffine3f>()->v;
      for (auto index : indices)
      {
        const box3f& cbounds = models->item(index.x).bounds();
        ospcommon::affine3f transform = worldTransform;
        if (index.y >= 0)
          transform = transform*transforms[index.y];
        if (cbounds.empty())
          continue;
        const vec3f lo = cbounds.lower;
        const vec3f hi = cbounds.upper;
        bounds.extend(xfmPoint(transform,vec3f(lo.x,lo.y,lo.z)));
        bounds.extend(xfmPoint(transform,vec3f(hi.x,lo.y,lo.z)));
        bounds.extend(xfmPoint(transform,vec3f(lo.x,hi.y,lo.z)));
        bounds.extend(xfmPoint(transform,vec3f(hi.x,hi.y,lo.z)));
        bounds.extend(xfmPoint(transform,vec3f(lo.x,lo.y,hi.z)));
        bounds.extend(xfmPoint(transform,vec3f(hi.x,lo.y,hi.z)));
        bounds.extend(xfmPoint(transform,vec3f(lo.x,hi.y,hi.z)));
        bounds.extend(xfmPoint(transform,vec3f(hi.x,hi.y,hi.z)));
      }
      for (auto child : children())
      {
        auto renderable = std::dynamic_pointer_cast<sg::Renderable>(child.second);
        if (renderable)
        {
          const box3f& cbounds = renderable->bounds();
          if (!cbounds.empty())
          {
            const vec3f lo = cbounds.lower;
            const vec3f hi = cbounds.upper;
            bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,lo.y,lo.z)));
            bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,lo.y,lo.z)));
            bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,hi.y,lo.z)));
            bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,hi.y,lo.z)));
            bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,lo.y,hi.z)));
            bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,lo.y,hi.z)));
            bounds.extend(xfmPoint(worldTransform,vec3f(lo.x,hi.y,hi.z)));
            bounds.extend(xfmPoint(worldTransform,vec3f(hi.x,hi.y,hi.z)));
          }
        }
      }
      return bounds;
    }

    void InstanceGroup::preCommit(RenderContext &ctx)
    {
      instanceDirty=true;
      if (ctx.currentTransform != cachedTransform)
        updateTransform(ctx);
    }

    void InstanceGroup::postCommit(RenderContext &)
    {
      child("bounds").setValue(computeBounds());
    }

    void InstanceGroup::preRender(RenderContext &ctx)
    {
      if (ctx.currentTransform != cachedTransform)
        instanceDirty = true;
      if (instanceDirty)
        updateInstances(ctx);
    }

    void InstanceGroup::postRender(RenderContext &ctx)
    {
      if (child("visible").value() == true
        && ctx.world && ctx.world->valueAs<OSPModel>())
      {
        for (auto instance : ospInstances)
          ospAddGeometry(ctx.world->valueAs<OSPModel>(), instance);
      }
    }

    void InstanceGroup::updateTransform(RenderContext &ctx)
    {
      //TODO: update transform with world transform
      worldTransform = ctx.currentTransform;
      instanceDirty=true;
    }

    void InstanceGroup::updateInstances(RenderContext &ctx)
    {
      ospInstances.resize(0);
      if (ctx.currentTransform != cachedTransform)
        updateTransform(ctx);

      if (!(hasChild("models") && hasChild("transforms") && hasChild("indices")))
        return;
      auto& indices = child("indices").nodeAs<DataVector2i>()->v;
      auto models = child("models").nodeAs<ModelList>();
      auto& transforms = child("transforms").nodeAs<DataVectorAffine3f>()->v;
      for (auto index : indices)
      {
        auto model = models->item(index.x).valueAs<OSPModel>();
        ospcommon::affine3f transform = worldTransform;
        if (index.y >= 0)
          transform = transform*transforms[index.y];
        if (model)
        {
          ospInstances.push_back(ospNewInstance(model,(osp::affine3f&)transform));
          ospCommit(ospInstances.back());
        }
      }
      instanceDirty=false;
      cachedTransform = worldTransform;
    }

    OSP_REGISTER_SG_NODE(Instance);
    OSP_REGISTER_SG_NODE(InstanceGroup);
    OSP_REGISTER_SG_NODE(ModelList);

  } // ::ospray::sg
} // ::ospray
