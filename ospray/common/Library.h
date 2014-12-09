//ospray stuff
#include "Managed.h"

//embree stuff
#include "common/sys/library.h"

namespace ospray {
  struct Library 
  {
    std::string   name;
    embree::lib_t lib;
  };

  void  loadLibrary(const std::string &name);
  void *getSymbol(const std::string &name);
}
