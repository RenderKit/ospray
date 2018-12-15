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

#include "../common/Node.h"
#include "../common/Serialization.h"

namespace ospray {
  namespace sg {

    /*! a camera node - the generic camera node */
    struct OSPSG_INTERFACE Camera : public sg::Node
    {
      Camera(const std::string &type);

      virtual std::string toString() const override;

      virtual void postCommit(RenderContext &ctx) override;

    protected:

      // camera type, i.e., "perspective", "orthographic", or "panoramic"
      const std::string type;
    };

    // Inlined Camera definitions /////////////////////////////////////////////

    inline Camera::Camera(const std::string &type) : type(type)
    {
      setValue(ospNewCamera(type.c_str()));
      createChild("pos", "vec3f", vec3f(0, -1, 0));
      // XXX SG is too restrictive: OSPRay cameras accept non-normalized directions
      createChild("dir", "vec3f", vec3f(0, 0, 0),
                       NodeFlags::required |
                       NodeFlags::gui_slider).setMinMax(vec3f(-1), vec3f(1));
      createChild("up", "vec3f", vec3f(0, 0, 1),NodeFlags::required);
    }

    inline std::string Camera::toString() const
    {
      return "ospray::sg::Camera";
    }

    inline void Camera::postCommit(RenderContext &)
    {
      ospCommit(valueAs<OSPCamera>());
    }

  } // ::ospray::sg
} // ::ospray
