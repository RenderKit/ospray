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

// sg
#include "sg/common/Node.h"
// ospcommon
#include "ospcommon/FileName.h"

namespace ospray {
  namespace sg {
    using ospcommon::FileName;

    /*! \brief C++ wrapper for a 2D Texture */
    struct Texture2D : public Node {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::viewer::sg::Texture2D"; };

      //! \brief load texture from given file. 
      /*! \detailed if file does not exist, or cannot be loaded for
          some reason, return NULL. Multiple loads from the same file
          will return the *same* texture object */
      static Ref<Texture2D> load(const FileName &fileName, const bool prefereLinear = false);
      virtual void render(RenderContext &ctx);

      //! texture size, in pixels
      vec2i       size;
      //! pixel data, in whatever format specified in 'texelType'
      void       *texel;
      //! format of each texel
      OSPTextureFormat texelType;
      
      OSPTexture2D ospTexture;
    };

  } // ::ospray::sg
} // ::ospray
