#include "volume.h"
#include "volume_ispc.h"
#include "naive32_ispc.h"
#include "bricked32_ispc.h"
#ifdef LOW_LEVEL_KERNELS
# include <immintrin.h>
#endif

namespace ospray {
  long long volumeMagicCookie = 0x123456789012345LL;

  const char *scalarTypeName[] = {
    "unorm8", "float", "double", "unspecified"
  };
  const int scalarTypeSize[] = {
    1,4,8,0
  };

  template<>
  NaiveVolume<uint8>::NaiveVolume(const vec3i &size, const uint8 *internalData)
    : Volume(size,ospray::UNORM8)
  { 
    ispcPtr = ispc::_Naive32Volume1uc_create((ispc::vec3i&)size,internalData); 
    int64_t dataSize;
    ispc::getInternalRepresentation(ispcPtr,dataSize,(void*&)data);
#ifdef LOW_LEVEL_KERNELS
    clampSize.x = size.x - 1e-6f;
    clampSize.y = size.y - 1e-6f;
    clampSize.z = size.z - 1e-6f;

    voxelOffset[0] = 0;
    voxelOffset[1] = 1;
    voxelOffset[2] = size.x;
    voxelOffset[3] = 1+size.x;
    voxelOffset[4] = 0+size.x*size.y;
    voxelOffset[5] = 1+size.x*size.y;
    voxelOffset[6] = 0+size.x+size.x*size.y;
    voxelOffset[7] = 1+size.x+size.x*size.y;
#endif
  }
  template<>
  BrickedVolume<uint8>::BrickedVolume(const vec3i &size, const uint8 *internalData)
    : Volume(size,ospray::UNORM8)
  { 
    ispcPtr = ispc::_Bricked32Volume1uc_create((ispc::vec3i&)size,internalData); 
  }
  template<>
  NaiveVolume<float>::NaiveVolume(const vec3i &size, const float *internalData)
    : Volume(size,ospray::FLOAT)
  { 
    ispcPtr = ispc::_Naive32Volume1f_create((ispc::vec3i&)size,internalData); 
    int64_t dataSize;
    ispc::getInternalRepresentation(ispcPtr,dataSize,(void*&)data);
  }
  template<>
  BrickedVolume<float>::BrickedVolume(const vec3i &size, const float *internalData)
    : Volume(size,ospray::FLOAT)
  { 
    ispcPtr = ispc::_Bricked32Volume1f_create((ispc::vec3i&)size,internalData); 
  }

  template<>
  std::string NaiveVolume<uint8>::toString() const { return "NaiveVolume<uint8>"; }
  template<>
  std::string BrickedVolume<uint8>::toString() const { return "BrickedVolume<uint8>"; }
  template<>
  std::string NaiveVolume<float>::toString() const { return "NaiveVolume<float>"; }
  template<>
  std::string BrickedVolume<float>::toString() const { return "BrickedVolume<float>"; }


  template<>
  void NaiveVolume<uint8>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const float *data)
  {
    uint8 t[long(size.x)*size.y*size.z];
    for (long i=0;i<long(size.x)*size.y*size.z;i++)
      t[i] = uint8(255.999f*data[i]);
    ispc::_Naive32Volume1uc_setRegion(ispcPtr,(const ispc::vec3i&)where,
                                       (const ispc::vec3i&)size,t);
  }

  template<>
  void BrickedVolume<uint8>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const float *data)
  {
    uint8 t[long(size.x)*size.y*size.z];
    for (long i=0;i<long(size.x)*size.y*size.z;i++)
      t[i] = uint8(255.999f*data[i]);
    ispc::_Bricked32Volume1uc_setRegion(ispcPtr,(const ispc::vec3i&)where,
                                       (const ispc::vec3i&)size,t);
  }
  template<>
  void NaiveVolume<uint8>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const uint8 *data)
  {
    ispc::_Naive32Volume1uc_setRegion(ispcPtr,(const ispc::vec3i&)where,
                                       (const ispc::vec3i&)size,data);
  }
  template<>
  void BrickedVolume<uint8>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const uint8 *data)
  {
    ispc::_Bricked32Volume1uc_setRegion(ispcPtr,(const ispc::vec3i&)where,
                                       (const ispc::vec3i&)size,data);
  }





  template<>
  void NaiveVolume<float>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const uint8 *data)
  {
    float t[long(size.x)*size.y*size.z];
    for (long i=0;i<long(size.x)*size.y*size.z;i++)
      t[i] = float(255.999f*data[i]);
    ispc::_Naive32Volume1f_setRegion(ispcPtr,(const ispc::vec3i&)where,
                                      (const ispc::vec3i&)size,t);
  }

  template<>
  void BrickedVolume<float>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const uint8 *data)
  {
    float t[long(size.x)*size.y*size.z];
    for (long i=0;i<long(size.x)*size.y*size.z;i++)
      t[i] = float(255.999f*data[i]);
    ispc::_Bricked32Volume1f_setRegion(ispcPtr,(const ispc::vec3i&)where,
                                       (const ispc::vec3i&)size,t);
  }
  template<>
  void NaiveVolume<float>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const float *data)
  {
    ispc::_Naive32Volume1f_setRegion(ispcPtr,(const ispc::vec3i&)where,
                                       (const ispc::vec3i&)size,data);
  }
  template<>
  void BrickedVolume<float>::setRegion(const vec3i &where, 
                                     const vec3i &size, 
                                     const float *data)
  {
    ispc::_Bricked32Volume1f_setRegion(ispcPtr,(const ispc::vec3i&)where,
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
    uint64 addr = ix + size.x*(iy + size.y*((uint64)iz));
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
    NOTIMPLEMENTED;
  }







  template<typename T>
  void loadRaw(Volume *volume, FILE *file, const vec3i &size)
  {
    assert(volume);
    assert(file);
    assert(size.x > 0);
    T t[size.x];
    for (int z=0;z<size.z;z++)
      for (int y=0;y<size.y;y++) {
        int n = fread(t,sizeof(T),size.x,file);
        if (n != size.x)
          throw std::runtime_error("ospray::loadRaw: read incomplete data");
        volume->setRegion(vec3i(0,y,z),vec3i(size.x,1,1),t);
      }
  }

  Volume *loadRaw(const char *fileName, 
                  ScalarType inputType, long sx, long sy, long sz,
                  long flags)
  {
    const vec3i size(sx,sy,sz);
    FILE *file = fopen(fileName,"rb");
    if (!file)
      throw std::runtime_error("ospray::loadRaw(): could not open volume '"
                               +std::string(fileName)+"'");

    long dataSize = sx *sy * sz * scalarTypeSize[inputType];

    Volume *volume = NULL;
    switch (flags & Volume::LAYOUT) {
    case Volume::NAIVE: 
      {
        switch(inputType) {
        case ospray::UNORM8:
          volume = new NaiveVolume<uint8>(size);
          loadRaw<unsigned char>(volume,file,size); 
          break;
        default:
          throw std::runtime_error
            (std::string("ospray::loadRaw() not yet implemented for input data type ")
             +ospray::scalarTypeName[inputType]+" and naive layout");
        }; 
      } break;
    case Volume::BRICKED: 
      {
        switch(inputType) {
        case ospray::UNORM8:
          volume = new BrickedVolume<uint8>(size);
          loadRaw<unsigned char>(volume,file,size); 
          break;
        default:
          throw std::runtime_error
            (std::string("ospray::loadRaw() not yet implemented for input data type ")
             +ospray::scalarTypeName[inputType]+" and naive layout");
        }; 
      } break;
    default:
      throw std::runtime_error
        (std::string("ospray::loadRaw() not yet implemented given layout"));
    }
    fclose(file);
    return volume;
  }

  template<typename T>
  Volume *createVolume(const vec3i &size, 
                       const long layout,
                       const T *internal)
  {
    switch (layout & Volume::LAYOUT) {
    case Volume::BRICKED:
      return new BrickedVolume<T>(size,internal);
      break;
    case Volume::NAIVE:
      return new NaiveVolume<T>(size,internal);
      break;
    default:
      NOTIMPLEMENTED;
    }
  }

  Volume *createVolume(const vec3i &size, 
                       const ScalarType scalar,
                       const long layout,
                       const void *data = NULL)
  {
    switch (scalar) {
    case FLOAT:
      return createVolume<float>(size,layout,(const float*)data);
    default:
      NOTIMPLEMENTED;
    }
  }

  Volume *resampleVolume(Volume *src, const vec3i &newSize, const long flags)
  {
    Volume *volume = NULL;
#if 0
    if ((flags & Volume::LAYOUT) == Volume::BRICKED)
      { PING; 
      volume = new BrickedVolume<uint8>(newSize);
      }
    else
      volume = new NaiveVolume<uint8>(newSize);
#else
    PING;
    if ((flags & Volume::LAYOUT) == Volume::BRICKED)
      volume = new BrickedVolume<float>(newSize);
    else
      volume = new NaiveVolume<float>(newSize);
#endif
    ispc::resampleVolume(volume->inISPC(),src->inISPC());
    return volume;
  }



  struct VolumeTrailer {
    long  long magicCookie;
    ScalarType scalarType;
    int   layout;
    vec3i size;
  };
  void saveVolume(Volume *volume, const std::string &fileName)
  {
    assert(volume);
    assert(volume->inISPC());
    int64_t dataSize = 0;
    void *dataPtr = NULL;
    ispc::getInternalRepresentation(volume->inISPC(),dataSize,dataPtr);
    assert(dataSize > 0);
    assert(dataPtr != NULL);
    FILE *file = fopen(fileName.c_str(),"wb");
    if (!file) 
      throw std::runtime_error("could not open volume data file '"
                               +fileName+"' for writing");
    VolumeTrailer trailer;
    trailer.size = volume->size;
    trailer.layout = volume->layout();
    trailer.scalarType = volume->scalarType;
    trailer.magicCookie = volumeMagicCookie;
    int rc;
    rc = fwrite(dataPtr,dataSize,1,file);
    assert(rc == 1);
    rc = fwrite(&trailer,sizeof(trailer),1,file);
    assert(rc == 1);
    fclose(file);
  }

  Volume *loadVolume(const std::string &fileName)
  {
    FILE *file = fopen(fileName.c_str(),"rb");
    assert(file);

    VolumeTrailer trailer;
    fseek(file,-sizeof(trailer),SEEK_END);
    long long sz = ftell(file);
    PRINT(sz);
    fread(&trailer,sizeof(trailer),1,file);
    assert(trailer.magicCookie == volumeMagicCookie);
    PRINT(trailer.size);
    void *data = malloc(sz);
    fread(data,sz,1,file);
    Volume *volume
      = createVolume(trailer.size,trailer.scalarType,trailer.layout,data);
    fclose(file);
    return volume;
  }

  void WrapperVolume::commit()
  {
    /* this code current does all the loading internally in ospray -
       this should not be; see comments in WrapperVolume */
    const vec3i size = getParam3i("dimensions",vec3i(-1));
    const vec3i resampleSize = getParam3i("resample_dimensions",vec3i(-1));
    Assert(size.x > 0);
    Assert(size.y > 0);
    Assert(size.z > 0);
    const char *fileName = getParamString("filename",NULL);
    Assert(fileName);

    PING;
    PRINT(fileName);

    internalRep = loadRaw(fileName,UNORM8,size.x,size.y,size.z,Volume::BRICKED);

    PRINT(resampleSize);

    if (resampleSize.x > 0) {
      std::cout << "resampling volume to new size " << resampleSize << std::endl;
      // internalRep = resampleVolume(internalRep,resampleSize,Volume::BRICKED);
      internalRep = resampleVolume(internalRep,resampleSize,Volume::NAIVE);
    }
    ispcEquivalent = internalRep->inISPC();
  }
}
