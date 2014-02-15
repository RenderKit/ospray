#include "volume.h"
#include "volume_ispc.h"
#include "naive32_ispc.h"
#include "bricked32_ispc.h"

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
    int64 dataSize;
    ispc::getInternalRepresentation(ispcPtr,(long&)dataSize,(void*&)data);
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
    int64 dataSize;
    ispc::getInternalRepresentation(ispcPtr,(long&)dataSize,(void*&)data);
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
#ifdef OSPRAY_TARGET_AVX2
    PING;
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

    const float v100 = data[addr+size.y];
    const float v101 = data[addr+1+size.y];
    const float v110 = data[addr+size.x+size.y];
    const float v111 = data[addr+1+size.x+size.y];

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

    internalRep = loadRaw(fileName,UNORM8,size.x,size.y,size.z,Volume::BRICKED);

    if (resampleSize.x > 0) {
      std::cout << "resampling volume to new size " << resampleSize << std::endl;
      // internalRep = resampleVolume(internalRep,resampleSize,Volume::BRICKED);
      internalRep = resampleVolume(internalRep,resampleSize,Volume::NAIVE);
    }
    ispcEquivalent = internalRep->inISPC();
  }
}
