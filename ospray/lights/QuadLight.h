// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "Light.h"

namespace ospray {

  /*! a QuadLight is a virtual area light uniformly emitting from a rectangular
   * area into the positive half space */
  struct OSPRAY_SDK_INTERFACE QuadLight : public Light
  {
    QuadLight();
    virtual ~QuadLight() override = default;
    virtual std::string toString() const override;
    virtual void commit() override;

  private:
    vec3f position {0.f};       //!< world-space corner position of the light
    vec3f edge1 {1.f, 0.f, 0.f};//!< vectors to adjacent corners
    vec3f edge2 {0.f, 1.f, 0.f};//!< vectors to adjacent corners
    vec3f color {1.f};          //!< RGB color of the QuadLight
    float intensity {1.f};      //!< Amount of light emitted
  };

}// ::ospray
