// ospray
#include "Library.h"
// std
#include <dlfcn.h>

namespace ospray {

  std::vector<Library*> loadedLibs;

  void  loadLibrary(const std::string &_name)
  {
#ifdef OSPRAY_TARGET_MIC
    std::string name = _name+"_mic";
#else
    std::string name = _name;
#endif

    for (int i=0;i<loadedLibs.size();i++)
      if (loadedLibs[i]->name == name)
        // lib already loaded.
        return;

    Library *lib = new Library;
    lib->name = name;
    lib->lib = embree::openLibrary(name);
    if (lib->lib == NULL)
      throw std::runtime_error("could not open module lib "+name);
    
    loadedLibs.push_back(lib);
  }
  void *getSymbol(const std::string &name)
  {
    for (int i=0;i<loadedLibs.size();i++) {
      void *sym = embree::getSymbol(loadedLibs[i]->lib, name);
      if (sym) return sym;
    }

    // if none found in the loaded libs, try the default lib ...
    void *sym = dlsym(RTLD_DEFAULT,name.c_str());
    return sym;
  }

} // ::ospray
