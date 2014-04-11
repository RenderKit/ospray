#include "bricked32.h"
#include "bricked32_ispc.h"
#include "bricked32.ih"

#ifdef LOW_LEVEL_KERNELS
# include <immintrin.h>
#endif

namespace ospray {

  // inline int divRoundUp(int nom, int den)
  // {
  //   return (nom + den - 1)/den;
  // }

  template<typename T>
  void BrickedVolume<T>::allocate() 
  {
    Assert(this->data == NULL);
    Assert(this->size.x > 0);
    Assert(this->size.y > 0);
    Assert(this->size.z > 0);
    
    const int brickRes = 1<<BRICK_BITS;
    this->bricks.x = divRoundUp(this->size.x,brickRes);
    this->bricks.y = divRoundUp(this->size.y,brickRes);
    this->bricks.z = divRoundUp(this->size.z,brickRes);

    const int64 numBricks
      = ((int64)this->bricks.x) * this->bricks.y * this->bricks.z;
    const int brickSize   = brickRes*brickRes*brickRes;
    const int numElements = numBricks * brickSize;

    this->data = new T[numElements];
  }

  template<>
  ispc::_Volume *BrickedVolume<uint8>::createIE()
  {
    assert(ispcEquivalent == NULL);
    this->ispcEquivalent = ispc::_Bricked32Volume1uc_create((ispc::vec3i&)size,data);
    return (ispc::_Volume*)this->ispcEquivalent;
  }

  template<>
  ispc::_Volume *BrickedVolume<float>::createIE()
  {
    assert(ispcEquivalent == NULL);
    this->ispcEquivalent = ispc::_Bricked32Volume1f_create((ispc::vec3i&)size,data);
    return (ispc::_Volume*)this->ispcEquivalent;
  }

  template<>
  void BrickedVolume<uint8>::setRegion(const vec3i &where, 
                                              const vec3i &size, 
                                              const uint8 *data)
  {
    ispc::_Bricked32Volume1uc_setRegion((ispc::_Volume*)getIE(),
                                        (const ispc::vec3i&)where,
                                        (const ispc::vec3i&)size,data);
  }

  template<>
  void BrickedVolume<float>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const float *data)
  {
    ispc::_Bricked32Volume1f_setRegion((ispc::_Volume*)getIE(),
                                       (const ispc::vec3i&)where,
                                       (const ispc::vec3i&)size,data);
  }


  template<typename T>
  inline int BrickedVolume_getCoord(BrickedVolume<T> *volume,
                                    const vec3i &pos)
  {
    const int brick_x = pos.x >> BRICK_BITS;
    const int brick_y = pos.y >> BRICK_BITS;
    const int brick_z = pos.z >> BRICK_BITS;
    
    const int ofs_x = pos.x & ((1<<BRICK_BITS)-1);
    const int ofs_y = pos.y & ((1<<BRICK_BITS)-1);
    const int ofs_z = pos.z & ((1<<BRICK_BITS)-1);
    
    const int brickID
      = brick_x + volume->bricks.x * (brick_y + volume->bricks.y * brick_z);
    int coord = brickID;
    coord = (coord << BRICK_BITS) | ofs_z;
    coord = (coord << BRICK_BITS) | ofs_y;
    coord = (coord << BRICK_BITS) | ofs_x;
    return coord;
  }


  template<typename T>
  inline float getVoxel(BrickedVolume<T> *volume,
                            const vec3i &coord)
  {
    const T *data = volume->data;
    const int idx = BrickedVolume_getCoord(volume,coord);
    return data[idx];
  }

  template<typename T>
  float BrickedVolume<T>::lerpf(const vec3fa &pos)
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
  // template<>
  // float BrickedVolume<float>::lerpf(const vec3fa &pos)
  // { 
  //   NOTIMPLEMENTED;
  // }
  // template<>
  // float BrickedVolume<uint8>::lerpf(const vec3fa &pos)
  // { 
  //   NOTIMPLEMENTED;
  // }

  template<>
  vec3f BrickedVolume<float>::gradf(const vec3fa &pos)
  { 
    NOTIMPLEMENTED;
  }
  template<>
  vec3f BrickedVolume<uint8>::gradf(const vec3fa &pos)
  { 
    NOTIMPLEMENTED;
  }

  typedef BrickedVolume<uint8> BrickedVolume_uint8;
  typedef BrickedVolume<float> BrickedVolume_float;

  OSP_REGISTER_VOLUME(BrickedVolume_uint8,bricked32_uint8);
  OSP_REGISTER_VOLUME(BrickedVolume_float,bricked32_float);
  OSP_REGISTER_VOLUME(BrickedVolume_float,bricked32_float32);
}
