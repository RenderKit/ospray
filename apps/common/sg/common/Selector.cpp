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

#include "Selector.h"

namespace ospray {
  namespace sg {

    Selector::Selector()
    {
      createChild("index", "int", 0);
    }

    void Selector::preTraverse(RenderContext &ctx, const std::string& operation, bool& traverseChildren)
    {
      if (operation == "render")
      {
        traverseChildren = false;
         const int index = child("index").valueAs<int>();
         const int numChildren = properties.children.size();
         if (index < numChildren - 2 && index >= 0)
         {
           int i = 0;
           for(auto &child : properties.children)
           {
             if (child.second->name() != "index" && child.second->name() != "bounds")
             {
               if (i++ == index)
                 child.second->finalize(ctx);
             }
           }
         }
      }
      else
      {
        Node::preTraverse(ctx,operation, traverseChildren);
      }
    }

    OSP_REGISTER_SG_NODE(Selector);

  } // ::ospray::sg
} // ::ospray

