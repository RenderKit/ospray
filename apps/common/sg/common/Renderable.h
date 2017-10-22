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

#pragma once

#include "Node.h"

namespace ospray {
  namespace sg {

    // Base Node for all renderables //////////////////////////////////////////

    //! a Node with bounds and a render operation
    struct OSPSG_INTERFACE Renderable : public Node
    {
      Renderable();
      virtual ~Renderable() override = default;

      virtual std::string toString() const override;

      virtual box3f bounds() const override;

      box3f computeBounds() const;

      virtual void preTraverse(RenderContext &ctx,
                               const std::string& operation, bool& traverseChildren) override;
      virtual void postTraverse(RenderContext &ctx,
                                const std::string& operation) override;
      virtual void postCommit(RenderContext &ctx) override;

      // Interface for render traversals //

      virtual void preRender(RenderContext &) {}
      virtual void postRender(RenderContext &) {}
    };

  } // ::ospray::sg
} // ::ospray