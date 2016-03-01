// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ospray
#include "Library.h"
// std

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

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
#ifdef _WIN32
    void *sym = GetProcAddress(GetModuleHandle(0), name.c_str()); // look in exe (i.e. when linked statically)
    if (!sym) {
      MEMORY_BASIC_INFORMATION mbi;
      VirtualQuery(getSymbol, &mbi, sizeof(mbi)); // get handle to current dll via a known function
      sym = GetProcAddress((HINSTANCE)(mbi.AllocationBase), name.c_str()); // look in ospray.dll (i.e. when linked dynamically)
    }
#else
    void *sym = dlsym(RTLD_DEFAULT,name.c_str());
#endif
    return sym;
  }

} // ::ospray
