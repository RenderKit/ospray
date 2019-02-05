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

  /*! a DirectionalLight is a singular light which is infinitely distant and
   *  thus projects parallel rays of light across the entire scene */
  struct OSPRAY_SDK_INTERFACE DirectionalLight : public Light
  {
    DirectionalLight();
    virtual ~DirectionalLight() override = default;
    virtual std::string toString() const override;
    virtual void commit() override;

  private:
    vec3f direction {0.f, 0.f, 1.f};//!< Direction of the emitted rays
    vec3f color {1.f};              //!< RGB color of the emitted light
    float intensity {1.f};          //!< Amount of light emitted
    float angularDiameter {0.f};    //!< Apparent size of the distant light, in degree (e.g. 0.53 for the sun)
  };

} // ::ospray
