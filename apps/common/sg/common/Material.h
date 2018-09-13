// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "NodeList.h"
#include "../texture/Texture.h"

namespace ospray {
  namespace sg {

    /*! \brief Base class for all Material Types */
    struct OSPSG_INTERFACE Material : public Node
    {
      Material();

      /*! \brief returns a std::string with the c++ name of this class */
      virtual std::string toString() const override;

      virtual void preCommit(RenderContext &ctx) override;
      virtual void postCommit(RenderContext &ctx) override;

      //! a logical name, of no other useful meaning whatsoever
      std::string name;
      //! indicates the type of material/shader the renderer should use for
      //  these parameters
      std::string type;
      //! vector of textures used by the material
      // Carson: what is this?  seems to be used by RIVL.  Is this supposed to be map_Kd?
      // how do I use a vector of textures?
      std::vector<std::shared_ptr<Texture>> textures;

      OSPRenderer ospRenderer {nullptr};
    };

    using MaterialList = NodeList<Material>;

  } // ::ospray::sg
} // ::ospray
