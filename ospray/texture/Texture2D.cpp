// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

  Texture2D *Texture2D::createTexture(int sx, int sy, OSPDataType type, void *data, int flags) 
  {
    Texture2D *tx = new Texture2D;
    
    //NOTE: For now, we just copy the data and rely on the viewer to have logic for buffer sharing.
    int bpp = 0;
    //TODO:ISPC side memory for images isn't currently managed, this will leak!
    void *img = NULL;
    switch (type) {
    case OSP_UCHAR4:
      tx->ispcEquivalent = ispc::Texture2D_4uc_create(tx,sx,sy,data);
      bpp = 4;
      break;
    case OSP_UCHAR3:
      tx->ispcEquivalent = ispc::Texture2D_3uc_create(tx,sx,sy,data);
      bpp = 3;
      break;
    case OSP_FLOAT3:
      tx->ispcEquivalent = ispc::Texture2D_3f_create(tx,sx,sy,data);
      bpp = 3 * sizeof(float);
      break;
    case OSP_FLOAT3A:
      tx->ispcEquivalent = ispc::Texture2D_4f_create(tx,sx,sy,data);
      bpp = 4 * sizeof(float);
      break;
    default: throw std::runtime_error("Could not determine bytes per pixel in " __FILE__);
    }

    assert(tx->ispcEquivalent && "ispcEquivalent may not be null in Texture2D");
    
    const int bytes = sx * sy * bpp;
    tx->width = sx;
    tx->height = sy;
    tx->type = type;
    
    assert(data);
    
    //Do we really need to make this copy?
    if (flags & OSP_DATA_SHARED_BUFFER) {
      tx->data = data;
    } else {
      tx->data = new unsigned char[bytes];
      memcpy(tx->data, data, bytes);
    }
    
    tx->managedObjectType = OSP_TEXTURE;
    return tx;
  }
  
} // ::ospray
