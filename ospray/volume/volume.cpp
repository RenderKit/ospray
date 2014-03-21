// ospray stuff
#include "volume.h"
#include "../common/library.h"
// embree stuff
#include "common/sys/library.h"
// STL
#include <map>

namespace ospray {
  long long volumeMagicCookie = 0x123456789012345LL;

  const char *scalarTypeName[] = {
    "unorm8", "float", "double", "unspecified"
  };
  const int scalarTypeSize[] = {
    1,4,8,0
  };

  typedef Volume *(*creatorFct)();

  std::map<std::string, creatorFct> volumeRegistry;


  template<typename T>
  void StructuredVolume<T>::loadRAW(const vec3i &size, const char *fileName)
  {
    assert(fileName);
    FILE *file = fopen(fileName,"rb");
    assert(file);
    assert(size.x > 0);
    this->size = size;
    allocate();
    createIE();
    T t[this->size.x];
    for (int z=0;z<this->size.z;z++)
      for (int y=0;y<this->size.y;y++) {
        int n = fread(t,sizeof(T),this->size.x,file);
        if (n != this->size.x)
          throw std::runtime_error("ospray::loadRaw: read incomplete data");
        this->setRegion(vec3i(0,y,z),vec3i(this->size.x,1,1),t);
      }
  }


  template<typename T>
  void StructuredVolume<T>::commit()
  {
    const vec3i size = getParam3i("dimensions",vec3i(-1));
    // const vec3i resampleSize = getParam3i("resample_dimensions",vec3i(-1));
    Assert(size.x > 0);
    Assert(size.y > 0);
    Assert(size.z > 0);
    const char *fileName = getParamString("filename",NULL);
    Assert(fileName);

    PING;
    PRINT(fileName);
    loadRAW(size,fileName);
    
    // PRINT(resampleSize);

    // if (resampleSize.x > 0) {
    //   std::cout << "resampling volume to new size " << resampleSize << std::endl;
    //   // internalRep = resampleVolume(internalRep,resampleSize,Volume::BRICKED);
    //   internalRep = resampleVolume(internalRep,resampleSize,Volume::NAIVE);
    // }
    
    // ispcEquivalent = internalRep->inISPC();
  }

  Volume *Volume::createVolume(const char *_type)
  {
    char type[strlen(_type)+2];
    strcpy(type,_type);
    for (char *s = type; *s; ++s)
      if (*s == '-') *s = '_';

    std::map<std::string, Volume *(*)()>::iterator it = volumeRegistry.find(type);
    if (it != volumeRegistry.end())
      return it->second ? (it->second)() : NULL;
    
    if (ospray::logLevel >= 2) 
      std::cout << "#ospray: trying to look up volume type '" 
                << type << "' for the first time" << std::endl;

    std::string creatorName = "ospray_create_volume__"+std::string(type);
    creatorFct creator = (creatorFct)getSymbol(creatorName); //dlsym(RTLD_DEFAULT,creatorName.c_str());
    volumeRegistry[type] = creator;
    if (creator == NULL) {
      if (ospray::logLevel >= 1) 
        std::cout << "#ospray: could not find volume type '" << type << "'" << std::endl;
      return NULL;
    }
    return (*creator)();
  }

  template struct StructuredVolume<uint8>;
  template struct StructuredVolume<float>;
}
