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
#include "texture/Texture2D.h"

namespace ospray {

  /*! a SpotLight is a singular light emitting from a point uniformly into a
   *  cone of directions bounded by halfAngle */
  struct OSPRAY_SDK_INTERFACE HDRILight : public Light
  {
    HDRILight();
    virtual ~HDRILight() override;
    virtual std::string toString() const override;
    virtual void commit() override;

  private:
    vec3f up {0.f, 1.f, 0.f}; //!< up direction of the light in world-space
    vec3f dir {0.f, 0.f, 1.f};//!< direction to which the center of the envmap
                              //   will be mapped to (analog to panoramic camera)
//    vec3f position;           //!< TODO world-space position of the LocalHDRILight
//    float radius;             //!< TODO radius of LocalHDRILight
//    bool  mirror;             //!< TODO whether to mirror the map
    Texture2D *map {nullptr};//!< environment map in latitude / longitude format
    float intensity {1.f};   //!< Amount of light emitted
  };

} // ::ospray

