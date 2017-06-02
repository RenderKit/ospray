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

// sg components
#include "sg/common/Node.h"

namespace ospray {
  namespace sg {

    //! a transformation node
    struct OSPSG_INTERFACE Transform : public sg::Renderable
    {
      Transform();
      std::string toString() const override;

      virtual void preRender(RenderContext &ctx) override;
      virtual void postRender(RenderContext &ctx) override;

      //! \brief the actual (affine) transformation matrix
      AffineSpace3f xfm;
      ospcommon::affine3f cachedTransform;
      ospcommon::affine3f baseTransform;
      ospcommon::affine3f worldTransform;  
      ospcommon::affine3f oldTransform{ospcommon::one};
    };

  } // ::ospray::sg
} // ::ospray
  
