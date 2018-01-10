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

#pragma once

#include "Visitor.h"
#include "../common/Node.h"

namespace ospray {
  namespace sg {

    struct MarkAllAsModified : public Visitor
    {
      MarkAllAsModified() = default;

      bool operator()(Node &node, TraversalContext &ctx) override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline bool MarkAllAsModified::operator()(Node &node, TraversalContext &)
    {
      node.markAsModified();
      return true;
    }

  } // ::ospray::sg
} // ::ospray

