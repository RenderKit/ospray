/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "naive64.h"
#include "naive64_ispc.h"

namespace ospray {
  template<typename T>
  void Naive64Volume<T>::allocate() 
  {
    Assert(this->data == NULL);
    Assert(this->size.x > 0);
    Assert(this->size.y > 0);
    Assert(this->size.z > 0);
    this->data = new T[int64(this->size.x)*int64(this->size.y)*int64(this->size.z)];
  }
  
  template<>
  ispc::_Volume *Naive64Volume<uint8>::createIE()
  {
    assert(ispcEquivalent == NULL);
    this->ispcEquivalent = ispc::_Naive64Volume1uc_create((ispc::vec3i&)size,data);
    return (ispc::_Volume*)this->ispcEquivalent;
  }
  template<>
  ispc::_Volume *Naive64Volume<float>::createIE()
  {
    assert(ispcEquivalent == NULL);
    this->ispcEquivalent = ispc::_Naive64Volume1f_create((ispc::vec3i&)size,data);
    return (ispc::_Volume*)this->ispcEquivalent;
  }

  template<>
  void Naive64Volume<uint8>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const uint8 *data)
  {
    ispc::_Naive64Volume1uc_setRegion((ispc::_Volume*)getIE(),(const ispc::vec3i&)where,
                                      (const ispc::vec3i&)size,data);
  }
  
  template<>
  void Naive64Volume<float>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const float *data)
  {
    ispc::_Naive64Volume1f_setRegion((ispc::_Volume*)getIE(),(const ispc::vec3i&)where,
                                     (const ispc::vec3i&)size,data);
  }
  template<>
  float Naive64Volume<float>::lerpf(const vec3fa &pos)
  { 
    float clamped_x = std::max(0.f,std::min(pos.x*size.x,size.x-1.0001f));
    float clamped_y = std::max(0.f,std::min(pos.y*size.y,size.y-1.0001f));
    float clamped_z = std::max(0.f,std::min(pos.z*size.z,size.z-1.0001f));

    uint64 ix = uint64(clamped_x);
    uint64 iy = uint64(clamped_y);
    uint64 iz = uint64(clamped_z);
    
    const float fx = clamped_x - ix;
    const float fy = clamped_y - iy;
    const float fz = clamped_z - iz;

    uint64 addr = ix + size.x*(iy + size.y*((uint64)iz));

    const float v000 = data[addr];
    const float v001 = data[addr+1];
    const float v010 = data[addr+size.x];
    const float v011 = data[addr+1+size.x];

    const float v100 = data[addr+size.x*size.y];
    const float v101 = data[addr+1+size.x*size.y];
    const float v110 = data[addr+size.x+size.x*size.y];
    const float v111 = data[addr+1+size.x+size.x*size.y];

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
  float Naive64Volume<uint8>::lerpf(const vec3fa &pos)
  { 
    float clamped_x = std::max(0.f,std::min(pos.x*size.x,size.x-1.0001f));
    float clamped_y = std::max(0.f,std::min(pos.y*size.y,size.y-1.0001f));
    float clamped_z = std::max(0.f,std::min(pos.z*size.z,size.z-1.0001f));
    uint64 ix = uint64(clamped_x);
    uint64 iy = uint64(clamped_y);
    uint64 iz = uint64(clamped_z);
    
    const float fx = clamped_x - ix;
    const float fy = clamped_y - iy;
    const float fz = clamped_z - iz;
    // PRINT(size);
    // PRINT(pos);
    uint64 addr = ix + size.x*(iy + size.y*((uint64)iz));
    // PRINT(addr);

    const float uint2float = 1.f/255.f;
    const float v000 = uint2float*data[addr];
    const float v001 = uint2float*data[addr+1];
    const float v010 = uint2float*data[addr+size.x];
    const float v011 = uint2float*data[addr+1+size.x];

    const float v100 = uint2float*data[addr+size.x*size.y];
    const float v101 = uint2float*data[addr+1+size.x*size.y];
    const float v110 = uint2float*data[addr+size.x+size.x*size.y];
    const float v111 = uint2float*data[addr+1+size.x+size.x*size.y];

    const float v00 = v000 + fx * (v001-v000);
    const float v01 = v010 + fx * (v011-v010);
    const float v10 = v100 + fx * (v101-v100);
    const float v11 = v110 + fx * (v111-v110);
    
    const float v0 = v00 + fy * (v01-v00);
    const float v1 = v10 + fy * (v11-v10);
    
    const float v = v0 + fz * (v1-v0);
    
    return v;
  }

  typedef Naive64Volume<uint8> Naive64Volume_uint8;
  typedef Naive64Volume<float> Naive64Volume_float;

  OSP_REGISTER_VOLUME(Naive64Volume_uint8,naive64_uint8);
  OSP_REGISTER_VOLUME(Naive64Volume_float,naive64_float);
  OSP_REGISTER_VOLUME(Naive64Volume_float,naive64_float32);
}

