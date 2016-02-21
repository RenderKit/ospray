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

#include "library.h"
#include "FileName.h"
#include "sysinfo.h"

// std
#include <time.h>
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h> // for GetSystemTime
#else
#  include <sys/time.h>
#  include <sys/times.h>
#  include <dlfcn.h>
#endif
// std
#include <vector>


namespace ospcommon {

  /*! type for shared library */
  typedef struct opaque_lib_t* lib_t;
	  
  struct Library 
  {
    std::string   name;
    lib_t lib;
  };

#if defined(__WIN32__)
  /* returns address of a symbol from the library */
  void* getSymbol(lib_t lib, const std::string& sym) {
    return GetProcAddress(HMODULE(lib),sym.c_str());
  }
#else
  /* returns address of a symbol from the library */
  void* getSymbol(lib_t lib, const std::string& sym) {
    return dlsym(lib,sym.c_str());
  }
#endif
  

  std::vector<Library*> loadedLibs;



#if defined(__WIN32__)
  /* opens a shared library */
  lib_t openLibrary(const std::string& file)
  {
    std::string fullName = file+".dll";
    FileName executable = getExecutableFileName();
    HANDLE handle = LoadLibrary((executable.path() + fullName).c_str());
    return lib_t(handle);
  }
#else
  /* opens a shared library */
  lib_t openLibrary(const std::string& file)
  {
#if defined(__MACOSX__)
    std::string fullName = "lib"+file+".dylib";
#else
    std::string fullName = "lib"+file+".so";
#endif
    void* lib = dlopen(fullName.c_str(), RTLD_NOW);
    if (lib) return lib_t(lib);
    FileName executable = getExecutableFileName();
    lib = dlopen((executable.path() + fullName).c_str(),RTLD_NOW);
    if (lib == NULL) throw std::runtime_error(dlerror());
    return lib_t(lib);
  }
#endif

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
    lib->lib = openLibrary(name);
    if (lib->lib == NULL)
      throw std::runtime_error("could not open module lib "+name);
    
    loadedLibs.push_back(lib);
  }
  void *getSymbol(const std::string &name)
  {
    for (int i=0;i<loadedLibs.size();i++) {
      void *sym = getSymbol(loadedLibs[i]->lib, name);
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

//   void *getSymbol(const std::string &name)
//   {
//     // for (int i=0;i<loadedLibs.size();i++) {
//     //   void *sym = embree_getSymbol(loadedLibs[i]->lib, name);
//     //   if (sym) return sym;
//     // }

//     // if none found in the loaded libs, try the default lib ...
// #ifdef _WIN32
//     void *sym = GetProcAddress(GetModuleHandle(0), name.c_str()); // look in exe (i.e. when linked statically)
//     if (!sym) {
//       MEMORY_BASIC_INFORMATION mbi;
//       VirtualQuery(getSymbol, &mbi, sizeof(mbi)); // get handle to current dll via a known function
//       sym = GetProcAddress((HINSTANCE)(mbi.AllocationBase), name.c_str()); // look in ospray.dll (i.e. when linked dynamically)
//     }
// #else
//     void *sym = dlsym(RTLD_DEFAULT,name.c_str());
// #endif
//     return sym;
//   }
}

