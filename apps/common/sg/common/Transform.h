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
    struct OSPSG_INTERFACE Transform : public sg::Node
    {
      Transform(const AffineSpace3f &xfm,
                const std::shared_ptr<sg::Node> &node);

      std::string toString() const override;

      /*! \brief return bounding box in world coordinates.

        This function can be used by the viewer(s) for calibrating
        camera motion, setting default camera position, etc. Nodes
        for which that does not apply can simpy return
        box3f(empty) */
      box3f bounds() const override;

      //! \brief the actual (affine) transformation matrix
      AffineSpace3f xfm;

      //! child node we're transforming
      std::shared_ptr<sg::Node> node;
    };

  } // ::ospray::sg
} // ::ospray
  
