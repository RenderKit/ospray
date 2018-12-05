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
#include "ospray/version.h"
#include "sysinfo.h"

// std
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/times.h>
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
        throw std::runtime_error(
            "windows cpuID doesn't support subfunc parameters");
      __cpuid((int *)&eax, func);
#else
      asm volatile("cpuid"
                   : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                   : "a"(func), "c"(subFunc));
#endif
    }

    uint32_t eax, ebx, ecx, edx;

    /*! avx512bw - only found in skylake x (and following models like
        canonlake etc) */
    static inline bool has_avx512bw()
    {
      return CpuID(7).ebx & (1 << 30);
    }

    /*! avx512bw - only found in knights series etc */
    static inline bool has_avx512er()
    {
      return CpuID(7).ebx & (1 << 27);
    }

    /*! avx512f - the common subset of all avx512 machines (both skl and knl) */
    static inline bool has_avx512f()
    {
      return CpuID(7).ebx & (1 << 16);
    }

    static inline bool has_avx2()
    {
      return CpuID(7).ebx & (1 << 5);
    }
    static inline bool has_avx()
    {
      return CpuID(1).ecx & (1 << 28);
    }
    static inline bool has_sse42()
    {
      return CpuID(1).ecx & (1 << 20);
    }
  };

  void *loadIsaLibrary(const std::string &name,
                       const std::string desiredISAname,
                       std::string &foundISAname,
                       std::string &foundPrec)
  {
    std::string precision       = "float";
    const char *use_double_flag = getenv("OSPRAY_USE_DOUBLES");
    if (use_double_flag && atoi(use_double_flag)) {
      precision = "double";
    }

    std::string file = name;
    void *lib        = nullptr;
#ifdef _WIN32
    std::string fullName = file + ".dll";
    lib                  = LoadLibrary(fullName.c_str());
#else
    std::string fullName =
        "lib" + file + "_" + desiredISAname + "_" + precision;

#if defined(__MACOSX__) || defined(__APPLE__)
    fullName += ".dylib";
#else
    fullName += ".so";
#endif

    lib = dlopen(fullName.c_str(), RTLD_NOW | RTLD_GLOBAL);

    if (!lib) {
      PRINT(dlerror());
      foundISAname = "";
    } else {
      std::cout << "#osp: loaded library *** " << fullName << " ***"
                << std::endl;
      foundISAname = desiredISAname;
      foundPrec = precision;
    }
#endif
    return lib;
  }

  /*! try loading the most isa-specific lib that is a) supported on
      this platform, and b) that can be found as a shared
      library. will return the most isa-specific lib (ie, if both avx
      and sse are available, and th ecpu supports at least avx (or
      more), it'll return avx, not sse*/
  void *tryLoadingMostIsaSpecificLib(const std::string &name,
                                     std::string &foundISA,
                                     std::string &foundPrec)
  {
    void *lib = NULL;

    // try 'native' first
    if ((lib = loadIsaLibrary(name, "native", foundISA, foundPrec)) != NULL)
      return lib;

    // no 'native found': assume build several isas explicitly for distribution:
    // try KNL:
    if (CpuID::has_avx512er() &&
        (lib = loadIsaLibrary(name, "knl", foundISA, foundPrec)))
      return lib;
    // try SKL:
    if (CpuID::has_avx512bw() &&
        (lib = loadIsaLibrary(name, "skx", foundISA, foundPrec)))
      return lib;
    // try avx2:
    if (CpuID::has_avx2() &&
        (lib = loadIsaLibrary(name, "avx2", foundISA, foundPrec)))
      return lib;
    // try avx1:
    if (CpuID::has_avx() &&
        (lib = loadIsaLibrary(name, "avx", foundISA, foundPrec)))
      return lib;
    // try sse4.2:
    if (CpuID::has_sse42() &&
        (lib = loadIsaLibrary(name, "sse4", foundISA, foundPrec)))
      return lib;

    // couldn't find any hardware-specific libs - return null, and let
    // caller try to load a generic, non-isa specific lib instead
    return NULL;
  }

  Library::Library(const std::string &name) : libraryName(name)
  {
    std::string file = name;
    std::string errorMsg;
#ifdef _WIN32
    std::string fullName = file + ".dll";
    lib                  = LoadLibrary(fullName.c_str());
    if (lib == nullptr) {
      DWORD err = GetLastError();
      LPTSTR lpMsgBuf;
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    err,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&lpMsgBuf,
                    0,
                    NULL);

      errorMsg = lpMsgBuf;

      LocalFree(lpMsgBuf);
    }
#else
#if defined(__MACOSX__) || defined(__APPLE__)
    std::string fullName = "lib" + file + ".dylib";
#else
    std::string fullName = "lib" + file + ".so";
#endif
    lib = dlopen(fullName.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (lib == nullptr) {
      errorMsg = dlerror();  // remember original error
      // retry with SOVERSION in case symlinks are missing
      std::string soversion(TOSTRING(OSPRAY_SOVERSION));
#if defined(__MACOSX__) || defined(__APPLE__)
      fullName = "lib" + file + "." + soversion + ".dylib";
#else
      fullName += "." + soversion;
#endif
      lib = dlopen(fullName.c_str(), RTLD_NOW | RTLD_GLOBAL);
    }
#endif

    // do NOT try to find the library in another location
    // if you want that use LD_LIBRARY_PATH or equivalents

    if (lib == nullptr) {
      std::string foundISA, foundPrec;
      lib = tryLoadingMostIsaSpecificLib(name, foundISA, foundPrec);
      if (lib) {
        std::cout << "#osp: found isa-specific lib for library " << name
                  << ", most specific ISA=" << foundISA
                  << ", using precision=" << foundPrec << std::endl;
        return;
      }

      throw std::runtime_error("could not open module lib " + name + ": " +
                               errorMsg);
    }
  }

  Library::~Library()
  {
    if (freeLibOnDelete) {
#ifdef _WIN32
      FreeLibrary((HMODULE)lib);
#else
      dlclose(lib);
#endif
    }
  }

  Library::Library(void *const _lib)
      : libraryName("<pre-loaded>"), lib(_lib), freeLibOnDelete(false)
  {
  }

  void *Library::getSymbol(const std::string &sym) const
  {
#ifdef _WIN32
    return GetProcAddress((HMODULE)lib, sym.c_str());
#else
    return dlsym(lib, sym.c_str());
#endif
  }

  std::unique_ptr<LibraryRepository> LibraryRepository::instance;

  LibraryRepository *LibraryRepository::getInstance()
  {
    if (instance.get() == nullptr)
      instance = std::unique_ptr<LibraryRepository>(new LibraryRepository);

    return instance.get();
  }

  void LibraryRepository::cleanupInstance()
  {
    LibraryRepository::instance.reset();
  }

  LibraryRepository::~LibraryRepository()
  {
    for (auto &l : repo)
      delete l.second;
  }

  void LibraryRepository::add(const std::string &name)
  {
    if (libraryExists(name))
      return;  // lib already loaded.

    repo[name] = new Library(name);
  }

  void *LibraryRepository::getSymbol(const std::string &name) const
  {
    void *sym = nullptr;
    for (auto lib = repo.cbegin(); sym == nullptr && lib != repo.end(); ++lib)
      sym = lib->second->getSymbol(name);

    return sym;
  }

  void LibraryRepository::addDefaultLibrary()
  {
    // already populate the repo with "virtual" libs, representing the default
    // OSPRay core lib
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
}  // namespace ospcommon
