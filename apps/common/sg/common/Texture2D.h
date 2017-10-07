// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "sg/common/Data.h"
// ospcommon
#include "ospcommon/FileName.h"

namespace ospray {
  namespace sg {

    /*! \brief C++ wrapper for a 2D Texture */
    struct OSPSG_INTERFACE Texture2D : public Node
    {
      /*! constructor */
      Texture2D();

      virtual void preCommit(RenderContext &ctx) override;

      /*! \brief returns a std::string with the c++ name of this class */
      std::string toString() const override;

      //! \brief load texture from given file.
      /*! \detailed if file does not exist, or cannot be loaded for
          some reason, return NULL. Multiple loads from the same file
          will return the *same* texture object */
      static std::shared_ptr<Texture2D> load(const FileName &fileName,
                                             const bool prefereLinear = false);

      //! texture size, in pixels
      vec2i size {-1};
      int channels{0};
      int depth{0};
      bool preferLinear{false};

      //! format of each texel
      OSPTextureFormat texelType {OSP_TEXTURE_FORMAT_INVALID};

      std::shared_ptr<sg::DataArray1uc> texelData;
      void* data{nullptr};
    };

  } // ::ospray::sg
} // ::ospray
