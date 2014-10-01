/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "texture2d.h"
#include "texture2d_ispc.h"

// #include "image3c_ispc.h"
// #include "image3ca_ispc.h"
// #include "image3f_ispc.h"
// #include "image3fa_ispc.h"
// #include "nearestneighbor_ispc.h"

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
      // tx->ispcEquivalent = ispc::Texture2D_3ca_bilinear_create(this,img);
      // img = ispc::Image3ca__new(width, height, (uint32_t*)data, true);
      break;
      // case OSP_UCHAR3:
      //   bpp = 3;
      //   img = ispc::Image3c__new(width, height, (ispc::vec3uc*)data, true);
      //   break;
    case OSP_UCHAR3:
      tx->ispcEquivalent = ispc::Texture2D_3uc_create(tx,sx,sy,data);
      bpp = 3;
      // tx->ispcEquivalent = ispc::Texture2D_3ca_bilinear_create(this,img);
      // img = ispc::Image3ca__new(width, height, (uint32_t*)data, true);
      break;
      // case OSP_UCHAR3:
      //   bpp = 3;
      //   img = ispc::Image3c__new(width, height, (ispc::vec3uc*)data, true);
      //   break;
    case OSP_FLOAT3:
      tx->ispcEquivalent = ispc::Texture2D_3f_create(tx,sx,sy,data);
      bpp = 3 * sizeof(float);
      // img = ispc::Image3f__new(width, height, (ispc::vec3f*)data, true);
      break;
    case OSP_FLOAT3A:
      tx->ispcEquivalent = ispc::Texture2D_4f_create(tx,sx,sy,data);
      bpp = 4 * sizeof(float);
      // img = ispc::Image3fa__new(width, height, (ispc::vec3fa*)data, true);
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
    
    return tx;
  }
  
}
