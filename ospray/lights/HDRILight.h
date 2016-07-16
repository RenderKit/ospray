// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
  class HDRILight : public Light {
    public:
      HDRILight();
      ~HDRILight();

      //! toString is used to aid in printf debugging
      virtual std::string toString() const { return "ospray::HDRILight"; }

      //! Copy understood parameters into class members
      virtual void commit();

    private:
      vec3f up;                 //!< up direction of the light in world-space
      vec3f dir;                //!< direction to which the center of the envmap
                                //   will be mapped to (analog to panoramic camera)
//      vec3f position;           //!< TODO world-space position of the LocalHDRILight
//      float radius;             //!< TODO radius of LocalHDRILight
//      bool  mirror;             //!< TODO whether to mirror the map
      Texture2D *map;           //!< environment map in latitude / longitude format
      float intensity;          //!< Amount of light emitted
  };

}

