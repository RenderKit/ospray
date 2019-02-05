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

#include "common/Material.h"
#include "texture/Texture2D.h"

namespace ospray {
  namespace simpleao {

    //! \brief Material used by the SimpleAO renderer
    /*! \detailed Since the SimpleAO Renderer only cares about a
        diffuse material component this material only stores diffuse
        and diffuse texture */
    struct Material : public ospray::Material {

      //! \brief Constructor
      Material();

      /*! \brief commit the object's outstanding changes
       *         (such as changed parameters etc) */
      virtual void commit() override;

      // -------------------------------------------------------
      // member variables
      // -------------------------------------------------------

      //! \brief diffuse material component, that's all we care for
      vec3f Kd;

      //! \brief diffuse texture, if available
      Ref<Texture2D> map_Kd;
    };
  }//namespace ospray::simpleao
}//namespace ospray
