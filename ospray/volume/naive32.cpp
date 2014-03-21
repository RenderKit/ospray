#include "naive32.h"
#include "naive32_ispc.h"
#include "importers.h"

namespace ospray {
  template<typename T>
  void NaiveVolume<T>::allocate() 
  {
    Assert(data == NULL);
    Assert(size.x > 0);
    Assert(size.y > 0);
    Assert(size.z > 0);
    this->data = new T[int64(this->size.x)*int64(this->size.y)*int64(this->size.z)];
  }

  // template<typename T>
  // void NaiveVolume<T>::commit()
  // {
  //   size = getParam3i("dimensions",vec3i(-1));
  //   allocate();
  //   Assert(size.x > 0);
  //   Assert(size.y > 0);
  //   Assert(size.z > 0);
  //   const char *fileName = getParamString("filename",NULL);
  //   if (fileName) {
  //     loadRAW(fileName,this);
  //     return;
  //   }
  // }

  // template<>
  // void NaiveVolume<uint8>::setRegion(const vec3i &where, 
  //                                           const vec3i &size, 
  //                                           const float *data)
  // {
  //   uint8 t[long(size.x)*size.y*size.z];
  //   for (long i=0;i<long(size.x)*size.y*size.z;i++)
  //     t[i] = uint8(255.999f*data[i]);
  //   ispc::_Naive32Volume1uc_setRegion(getIE(),(const ispc::vec3i&)where,
  //                                      (const ispc::vec3i&)size,t);
  // }
  
  template<>
  ispc::_Volume *NaiveVolume<uint8>::createIE()
  {
    assert(ispcEquivalent == NULL);
    this->ispcEquivalent = ispc::_Naive32Volume1uc_create((ispc::vec3i&)size,data);
    return (ispc::_Volume*)this->ispcEquivalent;
  }
  template<>
  ispc::_Volume *NaiveVolume<float>::createIE()
  {
    assert(ispcEquivalent == NULL);
    this->ispcEquivalent = ispc::_Naive32Volume1f_create((ispc::vec3i&)size,data);
    return (ispc::_Volume*)this->ispcEquivalent;
  }

  template<>
  void NaiveVolume<uint8>::setRegion(const vec3i &where, 
                                            const vec3i &size, 
                                            const uint8 *data)
  {
    ispc::_Naive32Volume1uc_setRegion((ispc::_Volume*)getIE(),(const ispc::vec3i&)where,
                                      (const ispc::vec3i&)size,data);
  }
  
  // template<>
  // void NaiveVolume<float>::setRegion(const vec3i &where, 
  //                                    const vec3i &size, 
  //                                    const uint8 *data)
  // {
  //   float t[long(size.x)*size.y*size.z];
  //   for (long i=0;i<long(size.x)*size.y*size.z;i++)
  //     t[i] = float(255.999f*data[i]);
  //   ispc::_Naive32Volume1f_setRegion(getIE(),(const ispc::vec3i&)where,
  //                                     (const ispc::vec3i&)size,t);
  // }

  template<>
  void NaiveVolume<float>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const float *data)
  {
    ispc::_Naive32Volume1f_setRegion((ispc::_Volume*)getIE(),(const ispc::vec3i&)where,
                                     (const ispc::vec3i&)size,data);
  }
  template<>
  float NaiveVolume<float>::lerpf(const vec3fa &pos)
  { 
#if LOW_LEVEL_KERNELS && defined(OSPRAY_TARGET_AVX2)
    const __m128 pos4
      = _mm_min_ps(_mm_max_ps((__m128&)pos,_mm_setzero_ps()),(__m128&)clampSize);
    const __m128i idx4 = _mm_cvttps_epi32(pos4);
    const __m128 f4 = _mm_sub_ps(pos4,_mm_cvtepi32_ps(idx4));

    float clamped_x = ((float*)&pos4)[0];
    float clamped_y = ((float*)&pos4)[1];
    float clamped_z = ((float*)&pos4)[2];
    // PRINT(pos);
    uint32 ix = ((int*)&idx4)[0];//uint32(clamped_x);
    uint32 iy = ((int*)&idx4)[1];//uint32(clamped_y);
    uint32 iz = ((int*)&idx4)[2];//uint32(clamped_z);
    
    const float fx = ((float*)&f4)[0];//clamped_x - ix;
    const float fy = ((float*)&f4)[1];//clamped_y - iy;
    const float fz = ((float*)&f4)[2];//clamped_z - iz;
    // PRINT(ix);
    // PRINT(iy);
    // PRINT(iz);
    uint64 addr = ix + this->size.x*(iy + this->size.y*((uint64)iz));
    // PRINT(addr);

    __m256 v8 = _mm256_i32gather_ps((const float*)addr,(__m256i&)voxelOffset,_MM_SCALE_4);
    // __m256 v000 = _mm256_set1_ps(*(float*)addr);
    // v = _mm256_sub_ps(v,
    const float v000 = ((float*)&v8)[0];//(data[addr];
    const float v001 = ((float*)&v8)[1];//data[addr+1];
    const float v010 = ((float*)&v8)[2];//data[addr+size.x];
    const float v011 = ((float*)&v8)[3];//data[addr+1+size.x];

    const float v100 = ((float*)&v8)[4];//data[addr+size.x*size.y];
    const float v101 = ((float*)&v8)[5];//data[addr+1+size.x*size.y];
    const float v110 = ((float*)&v8)[6];//data[addr+size.x+size.x*size.y];
    const float v111 = ((float*)&v8)[7];//data[addr+1+size.x+size.x*size.y];

    const float v00 = v000 + fx * (v001-v000);
    const float v01 = v010 + fx * (v011-v010);
    const float v10 = v100 + fx * (v101-v100);
    const float v11 = v110 + fx * (v111-v110);
    
    const float v0 = v00 + fy * (v01-v00);
    const float v1 = v10 + fy * (v11-v10);
    
    const float v = v0 + fz * (v1-v0);

    return v;
#else
    float clamped_x = std::max(0.f,std::min(pos.x,size.x-1.0001f));
    float clamped_y = std::max(0.f,std::min(pos.y,size.y-1.0001f));
    float clamped_z = std::max(0.f,std::min(pos.z,size.z-1.0001f));
    // PRINT(pos);
    uint32 ix = uint32(clamped_x);
    uint32 iy = uint32(clamped_y);
    uint32 iz = uint32(clamped_z);
    
    const float fx = clamped_x - ix;
    const float fy = clamped_y - iy;
    const float fz = clamped_z - iz;
    // PRINT(ix);
    // PRINT(iy);
    // PRINT(iz);
    uint64 addr = ix + size.x*(iy + size.y*((uint64)iz));
    // PRINT(addr);

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
#endif
  }
  template<>
  float NaiveVolume<uint8>::lerpf(const vec3fa &pos)
  { 
    float clamped_x = std::max(0.f,std::min(pos.x*size.x,size.x-1.0001f));
    float clamped_y = std::max(0.f,std::min(pos.y*size.y,size.y-1.0001f));
    float clamped_z = std::max(0.f,std::min(pos.z*size.z,size.z-1.0001f));
    uint32 ix = uint32(clamped_x);
    uint32 iy = uint32(clamped_y);
    uint32 iz = uint32(clamped_z);
    
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

  template struct StructuredVolume<uint8>;
  template struct StructuredVolume<float>;


  typedef NaiveVolume<uint8> NaiveVolume_uint8;
  typedef NaiveVolume<float> NaiveVolume_float;

  OSP_REGISTER_VOLUME(NaiveVolume_uint8,naive32_uint8);
  OSP_REGISTER_VOLUME(NaiveVolume_float,naive32_float);
  OSP_REGISTER_VOLUME(NaiveVolume_float,naive32_float32);
}

