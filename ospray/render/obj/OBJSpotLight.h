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

#include "ospray/lights/SpotLight.h"

namespace ospray {
  namespace obj {

    //!< OBJ renderer specific implementation of SpotLight
    struct OBJSpotLight : public SpotLight {
      OBJSpotLight();

      //! \brief common function to help printf-debugging
      virtual std::string toString() const { return "ospray::objrenderer::OBJPointLight"; }

      //! \brief commit the light's parameters
      virtual void commit();

      //! destructor, to clean up
      virtual ~OBJSpotLight();

      //Attenuation will be calculated as 1/( constantAttenuation + linearAttenuation * distance + quadraticAttenuation * distance * distance )
      float constantAttenuation;    //! Constant light attenuation
      float linearAttenuation;      //! Linear light attenuation
      float quadraticAttenuation;   //! Quadratic light attenuation
    };

  } // ::ospray::api
} // ::ospray
