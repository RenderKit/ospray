// ospray stuff
#include "geometry.h"
// embree stuff
#include "common/sys/library.h"
// stl stuff
#include <map>
// std c stuff
#include <dlfcn.h>

namespace ospray {
  typedef Geometry *(*creatorFct)();

  std::map<std::string, creatorFct> geometryRegistry;

  Geometry *Geometry::createGeometry(const char *type)
  {
    std::map<std::string, Geometry *(*)()>::iterator it = geometryRegistry.find(type);
    if (it != geometryRegistry.end())
      return it->second ? (it->second)() : NULL;
    
    if (ospray::logLevel >= 2) 
      std::cout << "#ospray: trying to look up geometry type '" 
                << type << "' for the first time" << std::endl;

    std::string creatorName = "ospray_create_geometry__"+std::string(type);
    creatorFct creator = (creatorFct)dlsym(RTLD_DEFAULT,creatorName.c_str());
    geometryRegistry[type] = creator;
    if (creator == NULL) {
      if (ospray::logLevel >= 1) 
        std::cout << "#ospray: could not find geometry type '" << type << "'" << std::endl;
      return NULL;
    }
    return (*creator)();
  }
};

