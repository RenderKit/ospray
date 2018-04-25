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

#include "AnimationController.h"

namespace ospray {
  namespace sg {

    AnimationController::AnimationController()
    {
      setValue(0.f); createChild("start","float", 0.0f);
      createChild("stop","float", 1.0f);
      createChild("time","float", 0.0f);
      createChild("step","float", 0.01f);
      createChild("enabled","bool",false);
    }

    void AnimationController::preTraverse(RenderContext &ctx, const std::string& operation, bool& traverseChildren)
    {
      Node::preTraverse(ctx,operation, traverseChildren);
      if (operation == "animate")
      {
        if (!child("enabled").valueAs<bool>())
        {
          traverseChildren = false;
          return;
        }
        float time = child("time").valueAs<float>();
        const float start = child("start").valueAs<float>();
        const float stop = child("stop").valueAs<float>();
        const float step = child("step").valueAs<float>();
        time += step;
        const float over = time - stop;
        if (over > 0.f)
          time = start + over;
        child("time") = time;
        ctx.time = time;
      }
    }


    OSP_REGISTER_SG_NODE(AnimationController);

  } // ::ospray::sg
} // ::ospray

