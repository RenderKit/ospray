// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "Renderable.h"

namespace ospray {
  namespace sg {

    struct OSPSG_INTERFACE Affine3f : public Node
    {
      Affine3f() : Node() { setValue(ospcommon::affine3f(ospcommon::one)); }
    };

    struct OSPSG_INTERFACE Transform : public Renderable
    {
      Transform();
      std::string toString() const override;

      void preCommit(RenderContext &ctx) override;
      void postCommit(RenderContext &ctx) override;
      void preRender(RenderContext &ctx) override;
      void postRender(RenderContext &ctx) override;

      //! \brief the actual (affine) transformation matrix
      ospcommon::affine3f worldTransform{ospcommon::one};  // computed transform
      ospcommon::affine3f cachedTransform{ospcommon::one};  // computed transform

    protected:
      void updateTransform(const ospcommon::affine3f& transform);
    };

  } // ::ospray::sg
} // ::ospray

