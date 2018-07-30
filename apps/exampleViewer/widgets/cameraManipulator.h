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

#include "ospcommon/common.h"
#include "ospcommon/box.h"
#include "ospcommon/AffineSpace.h"

#include "Imgui3dExport.h"

namespace ospray {
  //! dedicated namespace for 3D glut viewer widget
  namespace imgui3D {

    using namespace ospcommon;

    struct ImGui3DWidget;

    struct OSPRAY_IMGUI3D_INTERFACE Manipulator {
      Manipulator(ImGui3DWidget *widget)
        : widget(widget) {}

      virtual ~Manipulator() = default;

      // this is the fct that gets called when the mouse moved in the
      // associated window
      virtual void motion(ImGui3DWidget *widget);

      // helper functions called from the default 'motion' fct
      virtual void dragLeft(ImGui3DWidget * /*widget*/,
                            const vec2i & /*to*/,
                            const vec2i & /*from*/) {}
      virtual void dragRight(ImGui3DWidget * /*widget*/,
                             const vec2i & /*to*/,
                             const vec2i & /*from*/) {}
      virtual void dragMiddle(ImGui3DWidget * /*widget*/,
                              const vec2i & /*to*/,
                              const vec2i & /*from*/) {}

      ImGui3DWidget *widget {nullptr};
    };

    struct InspectCenter : public Manipulator
    {
      void dragLeft(ImGui3DWidget *widget,
                    const vec2i &to, const vec2i &from) override;
      void dragRight(ImGui3DWidget *widget,
                     const vec2i &to, const vec2i &from) override;
      void dragMiddle(ImGui3DWidget *widget,
                      const vec2i &to, const vec2i &from) override;
      InspectCenter(ImGui3DWidget *widget);
      void rotate(float du, float dv);

      vec3f pivot;
    };

    struct MoveMode : public Manipulator
    {
      void dragLeft(ImGui3DWidget *widget,
                    const vec2i &to, const vec2i &from) override;
      void dragRight(ImGui3DWidget *widget,
                     const vec2i &to, const vec2i &from) override;
      void dragMiddle(ImGui3DWidget *widget,
                      const vec2i &to, const vec2i &from) override;
      MoveMode(ImGui3DWidget *widget) : Manipulator(widget) {}
    };
  }
}

