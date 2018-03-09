// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <sys/times.h>
#  include <dlfcn.h>
#endif

namespace ospcommon {

  /*! helper class that executes a asm 'cpuid' instruction to query
      cpu capabilities */
  struct CpuID
  {
    CpuID(const uint32_t func, const uint32_t subFunc = 0)
    {
#ifdef _WIN32
      if (subFunc != 0)
        throw std::runtime_error("windows cpuID doesn't support subfunc parameters");
      __cpuid((int*)&eax,func);
#else
      asm volatile ("cpuid"
                    : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                    : "a"(func), "c"(subFunc)
                    );
#endif
    }

    uint32_t eax, ebx, ecx, edx;

    /*! avx512bw - only found in skylake x (and following models like
        canonlake etc) */
    static inline bool has_avx512bw() { return CpuID(7).ebx & (1<<30); }

    /*! avx512bw - only found in knights series etc */
    static inline bool has_avx512er() { return CpuID(7).ebx & (1<<27); }

    /*! avx512f - the common subset of all avx512 machines (both skl and knl) */
    static inline bool has_avx512f()  { return CpuID(7).ebx & (1<<16); }

    static inline bool has_avx2()     { return CpuID(7).ebx & (1<<5);  }
    static inline bool has_avx()      { return CpuID(1).ecx & (1<<28); }
    static inline bool has_sse42()    { return CpuID(1).ecx & (1<<20); }
  };

  void *loadIsaLibrary(const std::string &name,
                       const std::string desiredISAname,
                       std::string &foundISAname)
  {
    std::string file = name;
    void *lib = nullptr;
#ifdef _WIN32
    std::string fullName = file+".dll";
    lib = LoadLibrary(fullName.c_str());
#else
#if defined(__MACOSX__) || defined(__APPLE__)
    std::string fullName = "lib"+file+".dylib";
#else
    std::string fullName = "lib"+file+"_"+desiredISAname+".so";
#endif
    lib = dlopen(fullName.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!lib) {
      //      PRINT(dlerror());
      foundISAname = "";
    } else
      foundISAname = desiredISAname;
#endif
    return lib;
  }


  /*! try loading the most isa-specific lib that is a) supported on
      this platform, and b) that can be found as a shared
      library. will return the most isa-specific lib (ie, if both avx
      and sse are available, and th ecpu supports at least avx (or
      more), it'll return avx, not sse*/
  void *tryLoadingMostIsaSpecificLib(const std::string &name, std::string &foundISA)
  {
    void *lib = NULL;

    // try KNL:
    if (CpuID::has_avx512er() && (lib = loadIsaLibrary(name,"knl",foundISA))) return lib;
    // try SKL:
    if (CpuID::has_avx512bw() && (lib = loadIsaLibrary(name,"skx",foundISA))) return lib;
    // try avx2:
    if (CpuID::has_avx2() && (lib = loadIsaLibrary(name,"avx2",foundISA))) return lib;
    // try avx1:
    if (CpuID::has_avx() && (lib = loadIsaLibrary(name,"avx",foundISA))) return lib;
    // try sse4.2:
    if (CpuID::has_sse42() && (lib = loadIsaLibrary(name,"sse42",foundISA))) return lib;

    // couldn't find any hardware-specific libs - return null, and let
    // caller try to load a generic, non-isa specific lib instead
    return NULL;
  }

  Library::Library(const std::string& name)
  {
    std::string file = name;

    std::string foundISA;
    lib = tryLoadingMostIsaSpecificLib(name,foundISA);
    if (lib) {
      std::cout << "#osp: found isa-speicific lib for library " << name << ", most specific ISA=" << foundISA << std::endl;
      return;
    }

#ifdef _WIN32
    std::string fullName = file+".dll";
    lib = LoadLibrary(fullName.c_str());
#else
#if defined(__MACOSX__) || defined(__APPLE__)
    std::string fullName = "lib"+file+".dylib";
#else
    std::string fullName = "lib"+file+".so";
#endif
    lib = dlopen(fullName.c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif

    // iw: do NOT use this 'hack' that tries to find the
    // library in another location: first it shouldn't be used in
    // the first place (if you want a shared library, put it
    // into your LD_LIBRARY_PATH!; second, it messes up the 'real'
    // errors when the first library couldn't be opened (because
    // whatever error messaes there were - depedencies, missing
    // symbols, etc - get overwritten by that second dlopen,
    // which almost always returns 'file not found')

    if (lib == nullptr) {
#ifdef _WIN32
      // TODO: Must use GetLastError and FormatMessage on windows
      // to log out the error that occurred when calling LoadLibrary
      throw std::runtime_error("could not open module lib "+name);
#else
      const char* error = dlerror();
      throw std::runtime_error("could not open module lib "+name
          +" due to "+error);
#endif
    }
  }

  Library::Library(void* const _lib) : lib(_lib) {}

  void* Library::getSymbol(const std::string& sym) const
  {
#ifdef _WIN32
    return GetProcAddress((HMODULE)lib, sym.c_str());
#else
    return dlsym(lib, sym.c_str());
#endif
  }


  LibraryRepository* LibraryRepository::instance = nullptr;

  LibraryRepository* LibraryRepository::getInstance()
  {
    if (instance == nullptr)
      instance = new LibraryRepository;

    return instance;
  }

  void LibraryRepository::add(const std::string& name)
  {
    if (repo.find(name) != repo.end())
      return; // lib already loaded.

    repo[name] = new Library(name);
  }

  void* LibraryRepository::getSymbol(const std::string& name) const
  {
    void *sym = nullptr;
    for (auto lib = repo.cbegin(); sym == nullptr && lib != repo.end(); ++lib)
      sym = lib->second->getSymbol(name);

    return sym;
  }

  void LibraryRepository::addDefaultLibrary()
  {
    // already populate the repo with "virtual" libs, representing the default OSPRay core lib
#ifdef _WIN32
    // look in exe (i.e. when OSPRay is linked statically into the application)
    repo["exedefault"] = new Library(GetModuleHandle(0));

    // look in ospray.dll (i.e. when linked dynamically)
#if 0
    // we cannot get a function from ospray.dll, because this would create a
    // cyclic dependency between ospray.dll and ospray_common.dll

    // only works when ospray_common is linked statically into ospray
    const void * functionInOSPRayDLL = ospcommon::getSymbol;
    // get handle to current dll via a known function
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(functionInOSPRayDLL, &mbi, sizeof(mbi));
    repo["dlldefault"] = new Library(mbi.AllocationBase);
#else
    repo["ospray"] = new Library(std::string("ospray"));
#endif
#else
    repo["ospray"] = new Library(RTLD_DEFAULT);
#endif
  }

  bool LibraryRepository::libraryExists(const std::string &name) const
  {
    return repo.find(name) != repo.end();
  }

  LibraryRepository::LibraryRepository()
  {
  }
}
