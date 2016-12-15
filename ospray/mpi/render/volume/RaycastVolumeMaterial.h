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

#include "common/Material.h"
#include "texture/Texture2D.h"

#include "volume/Volume.h"

namespace ospray {

    typedef vec3f Color;

    //! Material used by RaycastVolumeRenderer.
    struct RaycastVolumeMaterial : public Material {

      RaycastVolumeMaterial();

      void commit();

      std::string toString() const;

      /*! opacity: 0 (transparent), 1 (opaque) */
      Texture2D *map_d;   float d;

      /*! diffuse  reflectance: 0 (none), 1 (full) */
      Texture2D *map_Kd;  Color Kd;

      /*! specular reflectance: 0 (none), 1 (full) */
      Texture2D *map_Ks;  Color Ks;

      /*! specular exponent: 0 (diffuse), infinity (specular) */
      Texture2D *map_Ns;  float Ns;

      /*! bump map */
      Texture2D *map_Bump;

      Ref<Volume> volume; //!< If provided, color will be mapped through this
                          //   volume's transfer function.
    };

    // Inlined member functions ///////////////////////////////////////////////

    inline std::string RaycastVolumeMaterial::toString() const
    {
      return "ospray::RaycastVolumeMaterial";
    }

} // ::ospray
