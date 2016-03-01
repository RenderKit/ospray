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

#include "Texture2D.h"
#include "Texture2D_ispc.h"

namespace ospray {

  Texture2D::~Texture2D()
  {
    if (!(flags & OSP_TEXTURE_SHARED_BUFFER)) delete[] (unsigned char *)data;
  }

  Texture2D *Texture2D::createTexture(int sx, int sy, OSPDataType type, void *data, int flags) 
  {
    Texture2D *tx = new Texture2D;

    tx->width = sx;
    tx->height = sy;
    tx->type = type;
    tx->flags = flags;
    tx->managedObjectType = OSP_TEXTURE;

    const size_t bytes = size_t(sx) * sy * sizeOf(type);

    assert(data);

    if (flags & OSP_TEXTURE_SHARED_BUFFER) {
      tx->data = data;
    } else {
      tx->data = bytes ? new unsigned char[bytes] : NULL;
      memcpy(tx->data, data, bytes);
    }

    switch (type) {
      case OSP_UCHAR4:
        tx->ispcEquivalent = ispc::Texture2D_4uc_create(sx,sy,tx->data,flags);
        break;
      case OSP_UCHAR3:
        tx->ispcEquivalent = ispc::Texture2D_3uc_create(sx,sy,tx->data,flags);
        break;
      case OSP_FLOAT:
        tx->ispcEquivalent = ispc::Texture2D_1f_create(sx,sy,tx->data,flags);
        break;
      case OSP_FLOAT3:
        tx->ispcEquivalent = ispc::Texture2D_3f_create(sx,sy,tx->data,flags);
        break;
      case OSP_FLOAT3A:
        tx->ispcEquivalent = ispc::Texture2D_4f_create(sx,sy,tx->data,flags);
        break;
      default: throw std::runtime_error("Could not determine bytes per pixel in " __FILE__);
    }

    return tx;
  }

} // ::ospray
