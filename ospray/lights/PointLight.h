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

#include "Light.h"

namespace ospray {

  //! a PointLight is a singular light emitting from a point uniformly into all directions
  struct OSPRAY_SDK_INTERFACE PointLight : public Light
  {
    PointLight();
    virtual ~PointLight() override = default;
    virtual std::string toString() const override;
    virtual void commit() override;

  private:
    vec3f position {0.f}; //!< world-space position of the light
    vec3f color {1.f};    //!< RGB color of the light
    float intensity {1.f};//!< Amount of light emitted
    float radius {0.f};   //!< Radius of SphereLight
  };

}// ::ospray
