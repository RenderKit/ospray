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

#include "TextureVolume.h"

namespace ospray {
  namespace sg {

    // TextureVolume definitions //////////////////////////////////////////////////

    TextureVolume::TextureVolume() : Texture("volume") {}

    std::string TextureVolume::toString() const
    {
      return "ospray::sg::TextureVolume";
    }

    void TextureVolume::postCommit(RenderContext &ctx)
    {
      auto ospTexture = valueAs<OSPTexture>();

      auto &volume = child("volume");
      ospSetObject(ospTexture, "volume", volume.valueAs<OSPVolume>());

      Texture::postCommit(ctx);
    }

    void TextureVolume::preTraverse(RenderContext &ctx,
                                    const std::string& operation,
                                    bool& traverseChildren)
    {
      traverseChildren = (operation != "render");
      Node::preTraverse(ctx, operation, traverseChildren);
    }

    OSP_REGISTER_SG_NODE(TextureVolume);

  } // ::ospray::sg
} // ::ospray
