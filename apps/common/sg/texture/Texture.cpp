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

#include "Texture.h"

namespace ospray {
  namespace sg {

    // Texture definitions ////////////////////////////////////////////////////

    Texture::Texture(const std::string &type)
    {
      setValue((OSPTexture)nullptr);
      ospTextureType = type;
    }

    std::string Texture::toString() const
    {
      return "ospray::sg::Texture";
    }

    void Texture::preCommit(RenderContext &)
    {
      auto ospTexture = valueAs<OSPTexture>();
      if (ospTexture != nullptr)
        return; // already created

      ospTexture = ospNewTexture(ospTextureType.c_str());
      setValue(ospTexture);
    }

    void Texture::postCommit(RenderContext &)
    {
      auto ospTexture = valueAs<OSPTexture>();
      ospCommit(ospTexture);
    }

  } // ::ospray::sg
} // ::ospray
