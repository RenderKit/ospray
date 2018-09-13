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

#include "Renderable.h"

namespace ospray {
  namespace sg {

    Renderable::Renderable()
    {
      createChild("bounds", "box3f", box3f(empty));
    }

    std::string Renderable::toString() const
    {
      return "ospray::sg::Renderable";
    }

    box3f Renderable::bounds() const
    {
      return child("bounds").valueAs<box3f>();
    }

    box3f Renderable::computeBounds() const
    {
      box3f cbounds = bounds();
      for (const auto &child : properties.children) {
        auto rChild = child.second->tryNodeAs<Renderable>();
        if (rChild.get() != nullptr)
          cbounds.extend(rChild->computeBounds());
      }
      child("bounds") = cbounds;
      return cbounds;
    }

    void Renderable::preTraverse(RenderContext &ctx,
                                 const std::string& operation,
                                 bool& traverseChildren)
    {
      Node::preTraverse(ctx,operation, traverseChildren);
      if (operation == "render")
        preRender(ctx);
    }

    void Renderable::postTraverse(RenderContext &ctx,
                                  const std::string& operation)
    {
      Node::postTraverse(ctx,operation);
      if (operation == "render")
        postRender(ctx);
    }

    void Renderable::postCommit(RenderContext &)
    {
      child("bounds") = computeBounds();
    }

  } // ::ospray::sg
} // ::ospray
