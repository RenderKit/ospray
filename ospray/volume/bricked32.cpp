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
    Assert(data == NULL);
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

  template<>
  float BrickedVolume<float>::lerpf(const vec3fa &pos)
  { 
    NOTIMPLEMENTED;
  }
  template<>
  float BrickedVolume<uint8>::lerpf(const vec3fa &pos)
  { 
    NOTIMPLEMENTED;
  }

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
