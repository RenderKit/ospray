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

#include "Texture2D.h"
#include "Texture2D_ispc.h"

#include "../common/Data.h"

namespace ospray {

  std::string Texture2D::toString() const
  {
    return "ospray::Texture2D";
  }

  void Texture2D::commit()
  {
    texData = getParamData("data");

    if (!texData || texData->numItems.z > 1)
      throw std::runtime_error(toString()
          + " must have 2D 'data' array using the first two dimensions.");

    const vec2i size = vec2i(texData->numItems.x, texData->numItems.y);
    if (!texData->compact()) {
      postStatusMsg(1)
          << toString()
          << " does currently not support strides, copying texture data.";

      auto data = new Data(texData->type, texData->numItems);
      data->copy(*texData, vec3ui(0));
      texData = data;
      data->refDec();
    }

    format = static_cast<OSPTextureFormat>(
        getParam<int>("format", OSP_TEXTURE_FORMAT_INVALID));
    filter = static_cast<OSPTextureFilter>(
        getParam<int>("filter", OSP_TEXTURE_FILTER_BILINEAR));

    if (sizeOf(format) != sizeOf(texData->type))
      throw std::runtime_error(toString() + ": 'format'='" + stringFor(format)
          + "' does not match type of 'data'='" + stringFor(texData->type)
          + "'!");

    this->ispcEquivalent = ispc::Texture2D_create(
        (ispc::vec2i &)size, texData->data(), format, filter);
  }

  OSP_REGISTER_TEXTURE(Texture2D, texture2d);

} // ::ospray
