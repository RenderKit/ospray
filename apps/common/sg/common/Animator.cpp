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

#include "Animator.h"

namespace ospray {
  namespace sg {

   template<typename T>
   Any lerp(float interp, const Any& value1, const Any& value2)
   {
     return T((1.f-interp)*value1.get<T>()+interp*value2.get<T>());
   }

    Animator::Animator()
    {
      setValue(0.f);
      createChild("start", "float", 0.f);
      createChild("stop", "float", 1.f);
    }

    void Animator::preCommit(RenderContext &)
    {
      if (!hasParent())
        return;
      if (!hasChild("value1")) { //TODO: support moving it?
        const std::string type = parent().type();
        createChild("value1", type);
        if (!hasChild("value2")) {
          createChild("value2", type);
          child("value2") = parent().value();
        }
        setValue(parent().value());
      }
      parent().setValue(value());
    }

    void Animator::preTraverse(RenderContext &ctx,
                               const std::string& operation,
                               bool& traverseChildren)
    {
      Node::preTraverse(ctx,operation, traverseChildren);
      if (operation == "animate") {
        const Any& value1 = child("value1").value();
        const Any& value2 = child("value2").value();
        const float start = child("start").valueAs<float>();
        const float stop = child("stop").valueAs<float>();
        const float duration = stop-start;
        if (ctx.time >= start && ctx.time <= stop) {
          float interp = (ctx.time-start)/duration;
          if (value1.is<float>())
            setValue(lerp<float>(interp, value1, value2));
          else if (value1.is<vec3f>())
            setValue(lerp<vec3f>(interp, value1, value2));
          else if (value1.is<int>())
            setValue(lerp<int>(interp, value1, value2));
        }
      }
    }


    OSP_REGISTER_SG_NODE(Animator);

  } // ::ospray::sg
} // ::ospray

