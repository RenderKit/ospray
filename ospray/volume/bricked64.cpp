/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "bricked64.h"
#include "bricked64_ispc.h"
#include "bricked64.ih"

namespace ospray {

  template<typename T>
  void PagedBrickedVolume<T>::allocate() 
  {
    Assert(this->data == NULL);
    Assert(this->size.x > 0);
    Assert(this->size.y > 0);
    Assert(this->size.z > 0);

    const int pageRes = PAGE_BRICK_RES;
    this->numPages.x = divRoundUp(this->size.x,pageRes);
    this->numPages.y = divRoundUp(this->size.y,pageRes);
    this->numPages.z = divRoundUp(this->size.z,pageRes);

    const int64 numPages
      = ((int64)this->numPages.x) * this->numPages.y * this->numPages.z;
    const int64 pageSize   = pageRes*pageRes*pageRes;
    const int64 numElements = numPages * pageSize;

    if (numElements)
      this->data = new T[numElements];
    else 
      this->data = NULL;
  }

  template<>
  ispc::_Volume *PagedBrickedVolume<unsigned char>::createIE()
  {
    ispcEquivalent = PagedBricked64Volume1uc_create((ispc::vec3i&)size,data);
    return (ispc::_Volume*)this->ispcEquivalent;
  }

  template<>
  ispc::_Volume *PagedBrickedVolume<float>::createIE()
  {
    assert(ispcEquivalent == NULL);
    this->ispcEquivalent = ispc::PagedBrickedVolume1f_create((ispc::vec3ui&)size,data);
    return (ispc::_Volume*)this->ispcEquivalent;
    
  }

  extern "C" void cSideLargeMemMalloc(void **ptr, int64 *numBytes)
  {
    PING;
    PRINT(ptr);
    PRINT(*numBytes);
    *ptr = malloc(*numBytes);
    PRINT(*ptr);
  }

  template<>
  void PagedBrickedVolume<uint8>::setRegion(const vec3i &where, 
                                              const vec3i &size, 
                                              const uint8 *data)
  {
    ispc::PagedBricked64Volume1uc_setRegion((ispc::_Volume*)getIE(),
                                            (const ispc::vec3i&)where,
                                            (const ispc::vec3i&)size,data);
  }
                                

  template<>
  void PagedBrickedVolume<float>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const float *data)
  {
    ispc::PagedBricked64Volume1f_setRegion((ispc::_Volume*)getIE(),
                                           (const ispc::vec3i&)where,
                                           (const ispc::vec3i&)size,data);
  }


  template<typename T>
  inline int PagedBrickedVolume_getCoord(PagedBrickedVolume<T> *volume,
                                    const vec3i &pos)
  {

    const uint32 PAGE_BRICK_MASK = (1<<(PAGE_BRICK_BITS-CACHE_BRICK_BITS))-1;
    const uint32 CACHE_BRICK_MASK = (1<<CACHE_BRICK_BITS)-1;

    // compute cell ID in cache brick
    const uint32 cellID_x = (pos.x & CACHE_BRICK_MASK);
    const uint32 cellID_y = (pos.y & CACHE_BRICK_MASK);
    const uint32 cellID_z = (pos.z & CACHE_BRICK_MASK);

    const uint32 brickID_x = (pos.x >> CACHE_BRICK_BITS) & PAGE_BRICK_MASK;
    const uint32 brickID_y = (pos.y >> CACHE_BRICK_BITS) & PAGE_BRICK_MASK;
    const uint32 brickID_z = (pos.z >> CACHE_BRICK_BITS) & PAGE_BRICK_MASK;
    
    const uint32 pageID_x = ((uint32&)pos.x) >> CACHE_BRICK_BITS;
    const uint32 pageID_y = ((uint32&)pos.y) >> CACHE_BRICK_BITS;
    const uint32 pageID_z = ((uint32&)pos.z) >> CACHE_BRICK_BITS;

    typename PagedBrickedVolume<T>::PageBrick *page
      = (typename PagedBrickedVolume<T>::PageBrick *)volume->data;
    const uint32 pageID
      = pageID_x + volume->numPages.x*(pageID_y + volume->numPages.y*pageID_z);
    return page[pageID]
      .brick[brickID_z][brickID_y][brickID_x]
      .cell[cellID_z][cellID_y][cellID_x];
  }


  template<typename T>
  inline float getVoxel(PagedBrickedVolume<T> *volume,
                            const vec3i &coord)
  {
    const T *data = volume->data;
    const int idx = PagedBrickedVolume_getCoord(volume,coord);
    return data[idx];
  }

  template<typename T>
  float PagedBrickedVolume<T>::lerpf(const vec3fa &pos)
  { 
    float clamped_x = std::max(0.f,std::min(pos.x*this->size.x,this->size.x-1.0001f));
    float clamped_y = std::max(0.f,std::min(pos.y*this->size.y,this->size.y-1.0001f));
    float clamped_z = std::max(0.f,std::min(pos.z*this->size.z,this->size.z-1.0001f));

    const int32 x0 = (int)(clamped_x);
    const int32 y0 = (int)(clamped_y);
    const int32 z0 = (int)(clamped_z);

    const float fx = clamped_x - (float)x0;
    const float fy = clamped_y - (float)y0;
    const float fz = clamped_z - (float)z0;
    
    const uint32 x1 = x0 + 1;
    const uint32 y1 = y0 + 1;
    const uint32 z1 = z0 + 1;

    const float v000 = getVoxel(this,vec3i(x0,y0,z0));
    const float v001 = getVoxel(this,vec3i(x1,y0,z0));
    const float v010 = getVoxel(this,vec3i(x0,y1,z0));
    const float v011 = getVoxel(this,vec3i(x1,y1,z0));
    const float v100 = getVoxel(this,vec3i(x0,y0,z1));
    const float v101 = getVoxel(this,vec3i(x1,y0,z1));
    const float v110 = getVoxel(this,vec3i(x0,y1,z1));
    const float v111 = getVoxel(this,vec3i(x1,y1,z1));
    
    const float v00 = v000 + fx * (v001-v000);
    const float v01 = v010 + fx * (v011-v010);
    const float v10 = v100 + fx * (v101-v100);
    const float v11 = v110 + fx * (v111-v110);
    
    const float v0 = v00 + fy * (v01-v00);
    const float v1 = v10 + fy * (v11-v10);
    
    const float v = v0 + fz * (v1-v0);
    
    return v;
  }

  template<>
  vec3f PagedBrickedVolume<float>::gradf(const vec3fa &pos)
  { 
    NOTIMPLEMENTED;
  }
  template<>
  vec3f PagedBrickedVolume<uint8>::gradf(const vec3fa &pos)
  { 
    NOTIMPLEMENTED;
  }

  typedef PagedBrickedVolume<uint8> PagedBrickedVolume_uint8;
  typedef PagedBrickedVolume<float> PagedBrickedVolume_float;

  OSP_REGISTER_VOLUME(PagedBrickedVolume_uint8,pageBricked64_uint8);
  OSP_REGISTER_VOLUME(PagedBrickedVolume_float,pageBricked64_float);
  OSP_REGISTER_VOLUME(PagedBrickedVolume_uint8,bricked64_uint8);
  OSP_REGISTER_VOLUME(PagedBrickedVolume_float,bricked64_float);
}
