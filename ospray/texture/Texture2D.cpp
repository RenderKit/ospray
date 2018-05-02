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

namespace ospray {

  Texture2D::~Texture2D()
  {
    if (!(flags & OSP_TEXTURE_SHARED_BUFFER))
      delete [] (unsigned char *)data;
  }

  std::string Texture2D::toString() const
  {
    return "ospray::Texture2D";
  }

  Texture2D *Texture2D::createTexture(const vec2i &_size,
      const OSPTextureFormat type, void *data, const int flags)
  {
    auto size = _size;
    Texture2D *tx = new Texture2D;

    tx->size = size;
    tx->type = type;
    tx->flags = flags;
    tx->managedObjectType = OSP_TEXTURE;

    const size_t bytes = sizeOf(type) * size.x * size.y;

    assert(data);

    if (flags & OSP_TEXTURE_SHARED_BUFFER) {
      tx->data = data;
    } else {
      tx->data = new unsigned char[bytes];
      memcpy(tx->data, data, bytes);
    }

    tx->ispcEquivalent = ispc::Texture2D_create((ispc::vec2i&)size,
                                                tx->data, type, flags);

    return tx;
  }

} // ::ospray
