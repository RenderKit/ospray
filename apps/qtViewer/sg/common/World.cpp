// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "sg/common/World.h"

namespace ospray {
  namespace sg {

    box3f World::getBounds() 
    {
      box3f bounds = empty;
      for (int i=0;i<node.size();i++)
        bounds.extend(node[i]->getBounds());
      return bounds;
    }

    //! serialize into given serialization state 
    void sg::World::serialize(sg::Serialization::State &state)
    {
      sg::Serialization::State savedState = state;
      {
        //state->serialization->object.push_back(Serialization::);
        for (size_t i=0; i<node.size(); i++)
          node[i]->serialize(state);
      }
      state = savedState;
    }

    void World::render(RenderContext &ctx)
    {
      ctx.world = this;

      if (ospModel)
        throw std::runtime_error("World::ospModel alrady exists!?");
      ospModel = ospNewModel();
      affine3f xfm = one;
      for (size_t i=0;i<node.size();i++) {
        node[i]->render(ctx);
      }
      ospCommit(ospModel);
    }

  }
}
