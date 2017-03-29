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

#undef NDEBUG

// scene graph
#include "SceneGraph.h"
#include "sg/common/Texture2D.h"
#include "sg/geometry/Spheres.h"

namespace ospray {
  namespace sg {

    /*! 'render' the nodes */
    void Group::render(RenderContext &ctx)
    {
      for (auto child : children) {
        assert(child);
        child->render(ctx); 
      }
    }
    
    box3f Group::bounds() const
    {
      box3f bounds = empty;
      for (auto child : children) {
        assert(child);
        bounds.extend(child->bounds());
      }
      return bounds;
    }

    void Node::serialize(sg::Serialization::State &state)
    { 
    }
    
    OSP_REGISTER_SG_NODE(Group);

  } // ::ospray::sg
} // ::ospray
