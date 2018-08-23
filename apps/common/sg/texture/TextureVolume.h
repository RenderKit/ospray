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

// sg
#include "Texture.h"

namespace ospray {
  namespace sg {

    /*! \brief C++ wrapper for a 2D Texture */
    struct OSPSG_INTERFACE TextureVolume : public Texture
    {
      /*! constructor */
      TextureVolume();
      virtual ~TextureVolume() override = default;

      void postCommit(RenderContext &ctx) override;

      void preTraverse(RenderContext &ctx,
                       const std::string& operation,
                       bool& traverseChildren) override;

      /*! \brief returns a std::string with the c++ name of this class */
      std::string toString() const override;
    };

  } // ::ospray::sg
} // ::ospray
