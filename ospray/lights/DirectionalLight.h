// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#include "ospray/lights/Light.h"
#include "ospray/common/Managed.h"

namespace ospray {

  /*! Base class for DirectionalLight objects, inherits from Light 
   *  Used for simulating a point light that is infinitely distant and projects parallel rays of light
   *  across the entire scene */
  struct DirectionalLight : public Light {
    //!< Construct a new DirectionalLight object
    DirectionalLight();

    //!< toString is used to aid in printf debugging
    virtual std::string toString() const { return "ospray::DirectionalLight"; }

    //!< Copy understood parameters into member parameters
    virtual void commit();

    vec3f color;          //!< RGB color of the emitted light
    vec3f direction;      //!< Direction of the emitted rays
  };

}
