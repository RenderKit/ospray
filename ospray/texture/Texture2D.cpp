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
    auto size = getParam<vec2i>("size", vec2i(-1, -1));

    if (size.x < 0 || size.y < 0) {
      std::stringstream ss;
      ss << "'size' param on Texture2D must be positive! got " << size;
      throw std::runtime_error(ss.str());
    }

    auto texData = getParamData("data", nullptr);

    if (!texData->data)
      throw std::runtime_error("no texel data provided to Texture2D");

    type  = static_cast<OSPTextureFormat>(
      getParam1i("type", OSP_TEXTURE_FORMAT_INVALID)
    );
    flags = getParam1i("flags", 0);

    const size_t numBytesExpected = sizeOf(type) * size.x * size.y;

    if (numBytesExpected != texData->numBytes) {
      std::stringstream ss;
      ss << "'size' and 'data' parameters disagree on memory size!\n";
      ss << "expected #bytes: " << numBytesExpected;
      ss << "   given #bytes: " << texData->numBytes;
      throw std::runtime_error(ss.str());
    }

    this->ispcEquivalent = ispc::Texture2D_create((ispc::vec2i&)size,
                                                  texData->data, type, flags);
  }

  OSP_REGISTER_TEXTURE(Texture2D, texture2d);
  OSP_REGISTER_TEXTURE(Texture2D, texture2D);
  OSP_REGISTER_TEXTURE(Texture2D, image2d);
  OSP_REGISTER_TEXTURE(Texture2D, image2D);
  OSP_REGISTER_TEXTURE(Texture2D, 2d);
  OSP_REGISTER_TEXTURE(Texture2D, 2D);

} // ::ospray
