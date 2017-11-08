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

namespace ospray {
  namespace sg {

    struct Node;

    // Data to track during traversal /////////////////////////////////////////

    struct TraversalContext
    {
      int level{0};
    };

    // Base node visitor interface ////////////////////////////////////////////

    struct Visitor
    {
      // NOTE: return value means "continue traversal"
      virtual bool visit(Node &node, TraversalContext &ctx) = 0;

      virtual ~Visitor() = default;
    };

  } // ::ospray::sg
} // ::ospray