// ospray
#include "material.h"
#include "common/library.h"
// stl
#include <map>

namespace ospray {

  typedef Material *(*creatorFct)();

  std::map<std::string, creatorFct> materialRegistry;


  /*! \brief creates an abstract material class of given type 
    
    The respective material type must be a registered material type
    in either ospray proper or any already loaded module. For
    material types specified in special modules, make sure to call
    ospLoadModule first. */
  Material *Material::createMaterial(const char *_type)
  {
    char type[strlen(_type)+2];
    strcpy(type,_type);
    for (char *s = type; *s; ++s)
      if (*s == '-') *s = '_';

    std::map<std::string, Material *(*)()>::iterator it = materialRegistry.find(type);
    if (it != materialRegistry.end())
      return it->second ? (it->second)() : NULL;
    
    if (ospray::logLevel >= 2) 
      std::cout << "#ospray: trying to look up material type '" 
                << type << "' for the first time" << std::endl;

    std::string creatorName = "ospray_create_material__"+std::string(type);
    creatorFct creator = (creatorFct)getSymbol(creatorName); //dlsym(RTLD_DEFAULT,creatorName.c_str());
    materialRegistry[type] = creator;
    if (creator == NULL) {
      if (ospray::logLevel >= 1) 
        std::cout << "#ospray: could not find material type '" << type << "'" << std::endl;
      return NULL;
    }
    return (*creator)();
  }

}
