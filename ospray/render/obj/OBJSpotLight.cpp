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

#include "OBJSpotLight.h"
#include "OBJSpotLight_ispc.h"

namespace ospray {
  namespace obj {

    //! Create a new OBJSpotLight object and ispc equivalent
    OBJSpotLight::OBJSpotLight()
      : SpotLight()
      , constantAttenuation(-1.f)
      , linearAttenuation(-1.f)
      , quadraticAttenuation(-1.f)
    {
      ispcEquivalent = ispc::OBJSpotLight_create(this);
    }

    //! Commit parameters to class members. Pass data on to ispc-side object
    void OBJSpotLight::commit() {
      //commit inherited params
      SpotLight::commit();

      constantAttenuation = getParam1f("attenuation.constant", 0.f);
      linearAttenuation = getParam1f("attenuation.linear", 0.f);
      quadraticAttenuation = getParam1f("attenuation.quadratic", 0.f);

      ispc::OBJSpotLight_set(getIE(),
                            (ispc::vec3f&)position,
                            (ispc::vec3f&)direction,
                            (ispc::vec3f&)color,
                            range,
                            halfAngle,
                            constantAttenuation,
                            linearAttenuation,
                            quadraticAttenuation);
    }

    //! Destroy an OBJSpotLight object
    OBJSpotLight::~OBJSpotLight() {} //TODO: Need to actually destruct the ispc side. Since we should 'never' have these on the stack, would it be safe to just put the destruct in directly?

    //Register the light type
    OSP_REGISTER_LIGHT(OBJSpotLight, OBJ_SpotLight);

  } // ::ospray::api
} // ::ospray

