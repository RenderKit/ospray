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

#include "Model.h"

#include "../visitor/MarkAllAsModified.h"

namespace ospray {
  namespace sg {

    Model::Model()
    {
      setValue((OSPModel)nullptr);
      createChild("dynamicScene", "bool", true);
      createChild("compactMode", "bool", false);
      createChild("robustMode", "bool", false);
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
      auto model = valueAs<OSPModel>();
      if (model) {
        ospRelease(model);
        child("dynamicScene").markAsModified();
        child("compactMode").markAsModified();
        child("robustMode").markAsModified();
      }
      model = ospNewModel();
      setValue(model);
      stashedModel = ctx.currentOSPModel;
      ctx.currentOSPModel = model;
    }

    void Model::postCommit(RenderContext &ctx)
    {
      auto model = valueAs<OSPModel>();
      ctx.currentOSPModel = model;

      //instancegroup caches render calls in commit.
      for (auto &child : properties.children)
        child.second->finalize(ctx);

      ospCommit(model);
      ctx.currentOSPModel = stashedModel;

      // reset bounding box
      child("bounds") = box3f(empty);
      computeBounds();
    }

    OSP_REGISTER_SG_NODE(Model);

  } // ::ospray::sg
} // ::ospray
