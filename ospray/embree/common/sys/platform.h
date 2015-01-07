// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#pragma once

#pragma warning (disable: 3280)
#pragma warning (disable: 177)
#pragma warning (disable: 2415)
#pragma warning (disable: 2407)
#pragma warning (disable: 2557) // message #2557: comparison between signed and unsigned operands
#pragma warning (disable: 82)   // message #82: storage class is not first

#include <stddef.h>
#include <assert.h>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <stdexcept>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
/// detect platform
////////////////////////////////////////////////////////////////////////////////

/* detect 32 or 64 platform */
#if defined(__x86_64__) || defined(__ia64__) || defined(_M_X64)
#define __X86_64__
#endif

/* detect Linux platform */
#if defined(linux) || defined(__linux__) || defined(__LINUX__)
#  if !defined(__LINUX__)
#     define __LINUX__
#  endif
#  if !defined(__UNIX__)
#     define __UNIX__
#  endif
#endif

/* detect FreeBSD platform */
#if defined(__FreeBSD__) || defined(__FREEBSD__)
#  if !defined(__FREEBSD__)
#     define __FREEBSD__
#  endif
#  if !defined(__UNIX__)
#     define __UNIX__
#  endif
#endif

/* detect Windows 95/98/NT/2000/XP/Vista/7 platform */
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)) && !defined(__CYGWIN__)
#  if !defined(__WIN32__)
#     define __WIN32__
#  endif
#endif

/* detect Cygwin platform */
#if defined(__CYGWIN__)
#  if !defined(__UNIX__)
#     define __UNIX__
#  endif
#endif

/* detect MAC OS X platform */
#if defined(__APPLE__) || defined(MACOSX) || defined(__MACOSX__)
#  if !defined(__MACOSX__)
#     define __MACOSX__
#  endif
#  if !defined(__UNIX__)
#     define __UNIX__
#  endif
#endif

/* try to detect other Unix systems */
#if defined(__unix__) || defined (unix) || defined(__unix) || defined(_unix)
#  if !defined(__UNIX__)
#     define __UNIX__
#  endif
#endif

#if defined (_DEBUG)
#define DEBUG
#endif

////////////////////////////////////////////////////////////////////////////////
/// Configurations
////////////////////////////////////////////////////////////////////////////////

#if defined(__WIN32__) 
#if defined(CONFIG_AVX)
  #if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    #define __SSE3__
    #define __SSSE3__
    #define __SSE4_1__
    #define __SSE4_2__
    #if !defined(__AVX__)
      #define __AVX__
    #endif
  #endif
#endif
#if defined(CONFIG_AVX2)
  #if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    #define __SSE3__
    #define __SSSE3__
    #define __SSE4_1__
    #define __SSE4_2__
    #if !defined(__AVX__)
      #define __AVX__
    #endif
    #if !defined(__AVX2__)
      #define __AVX2__
    #endif
#endif
#endif
//#define __USE_RAY_MASK__
//#define __USE_STAT_COUNTERS__
//#define __BACKFACE_CULLING__
#ifndef __INTERSECTION_FILTER__
#define __INTERSECTION_FILTER__
#endif
#ifndef __BUFFER_STRIDE__
#define __BUFFER_STRIDE__
#endif
//#define __SPINLOCKS__
//#define __LOG_TASKS__
#endif

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define __SSE__
#define __SSE2__
#endif

////////////////////////////////////////////////////////////////////////////////
/// Makros
////////////////////////////////////////////////////////////////////////////////

#ifdef __WIN32__
#define __dllexport extern "C" __declspec(dllexport)
#define __dllimport extern "C" __declspec(dllimport)
#else
#define __dllexport extern "C" __attribute__ ((visibility ("default")))
#define __dllimport extern "C"
#endif

#if defined(_WIN32) || defined(__EXPORT_ALL_SYMBOLS__)
#  define __hidden
#else
#  define __hidden __attribute__ ((visibility ("hidden")))
#endif

#ifdef __WIN32__
#undef __noinline
#define __noinline             __declspec(noinline)
//#define __forceinline          __forceinline
//#define __restrict             __restrict
#define __thread               __declspec(thread)
#define __aligned(...)           __declspec(align(__VA_ARGS__))
//#define __FUNCTION__           __FUNCTION__
#define debugbreak()           __debugbreak()

#else
#undef __noinline
#undef __forceinline
#define __noinline             __attribute__((noinline))
#define __forceinline          inline __attribute__((always_inline))
//#define __restrict             __restrict
//#define __thread               __thread
#define __aligned(...)           __attribute__((aligned(__VA_ARGS__)))
#define __FUNCTION__           __PRETTY_FUNCTION__
#define debugbreak()           asm ("int $3")
#endif

#ifdef __GNUC__
  #define MAYBE_UNUSED __attribute__((used))
#else
  #define MAYBE_UNUSED
#endif

#if defined(_MSC_VER)
  #define __restrict__
#endif

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define   likely(expr) (expr)
#define unlikely(expr) (expr)
#else
#define   likely(expr) __builtin_expect((expr),true )
#define unlikely(expr) __builtin_expect((expr),false)
#endif

/* compiler memory barriers */
#ifdef __GNUC__
#  define __memory_barrier() asm volatile("" ::: "memory")
#elif defined(__MIC__)
#define __memory_barrier()
#elif defined(__INTEL_COMPILER)
//#define __memory_barrier() __memory_barrier()
#elif  defined(_MSC_VER)
#  define __memory_barrier() _ReadWriteBarrier()
#endif

/* debug printing macros */
#define STRING(x) #x
#define TOSTRING(x) STRING(x)
#define PING std::cout << __FILE__ << " (" << __LINE__ << "): " << __FUNCTION__ << std::endl
#define PRINT(x) std::cout << STRING(x) << " = " << (x) << std::endl
#define PRINT2(x,y) std::cout << STRING(x) << " = " << (x) << ", " << STRING(y) << " = " << (y) << std::endl
#define PRINT3(x,y,z) std::cout << STRING(x) << " = " << (x) << ", " << STRING(y) << " = " << (y) << ", " << STRING(z) << " = " << (z) << std::endl
#define PRINT4(x,y,z,w) std::cout << STRING(x) << " = " << (x) << ", " << STRING(y) << " = " << (y) << ", " << STRING(z) << " = " << (z) << ", " << STRING(w) << " = " << (w) << std::endl

#define DBG_PRINT(x) std::cout << STRING(x) << " = " << (x) << std::endl
#define FATAL(x) { std::cout << "FATAL error in " << __FUNCTION__ << " : " << x << std::endl << std::flush; exit(0); }

#define THROW_RUNTIME_ERROR(str) \
  throw std::runtime_error(std::string(__FILE__) + " (" + std::stringOf(__LINE__) + "): " + std::string(str));

////////////////////////////////////////////////////////////////////////////////
/// Basic Types
////////////////////////////////////////////////////////////////////////////////

typedef          long long  int64;
typedef unsigned long long uint64;
typedef                int  int32;
typedef unsigned       int uint32;
typedef              short  int16;
typedef unsigned     short uint16;
typedef               char   int8;
typedef unsigned      char  uint8;

#ifdef __WIN32__
#if defined(__X86_64__)
typedef int64 ssize_t;
#else
typedef int32 ssize_t;
#endif
#endif

////////////////////////////////////////////////////////////////////////////////
/// Disable some compiler warnings
////////////////////////////////////////////////////////////////////////////////

#if defined(__INTEL_COMPILER)
#pragma warning(disable:265 ) // floating-point operation result is out of range
#pragma warning(disable:383 ) // value copied to temporary, reference to temporary used
#pragma warning(disable:869 ) // parameter was never referenced
#pragma warning(disable:981 ) // operands are evaluated in unspecified order
#pragma warning(disable:1418) // external function definition with no prior declaration
#pragma warning(disable:1419) // external declaration in primary source file
#pragma warning(disable:1572) // floating-point equality and inequality comparisons are unreliable
#pragma warning(disable:94  ) // the size of an array must be greater than zero
#pragma warning(disable:1599) // declaration hides parameter
#pragma warning(disable:424 ) // extra ";" ignored
#endif

#if defined(_MSC_VER)
#pragma warning(disable:4200) // nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable:4800) // forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable:4267) // '=' : conversion from 'size_t' to 'unsigned long', possible loss of data
#pragma warning(disable:4244) // 'argument' : conversion from 'ssize_t' to 'unsigned int', possible loss of data
#pragma warning(disable:4355) // 'this' : used in base member initializer list
#pragma warning(disable:4996) // 'std::copy': Function call with parameters that may be unsafe 
#pragma warning(disable:391 ) // '<=' : signed / unsigned mismatch
#pragma warning(disable:4018) // '<' : signed / unsigned mismatch

#endif

////////////////////////////////////////////////////////////////////////////////
/// Default Includes and Functions
////////////////////////////////////////////////////////////////////////////////

#include "sys/constants.h"
#include "sys/stl/string.h"

namespace embree
{
#define ALIGNED_STRUCT                                           \
  void* operator new(size_t size) { return alignedMalloc(size); }       \
  void operator delete(void* ptr) { alignedFree(ptr); }      \
  void* operator new[](size_t size) { return alignedMalloc(size); }  \
  void operator delete[](void* ptr) { alignedFree(ptr); }    \

#define ALIGNED_STRUCT_(align)                                           \
  void* operator new(size_t size) { return alignedMalloc(size,align); } \
  void operator delete(void* ptr) { alignedFree(ptr); }                 \
  void* operator new[](size_t size) { return alignedMalloc(size,align); } \
  void operator delete[](void* ptr) { alignedFree(ptr); }               \

#define ALIGNED_CLASS                                                \
  public:                                                            \
    ALIGNED_STRUCT                                                  \
  private:

#define ALIGNED_CLASS_(align)                                           \
 public:                                                               \
    ALIGNED_STRUCT_(align)                                              \
 private:
  
  /*! aligned allocation */
  void* alignedMalloc(size_t size, size_t align = 64);
  void alignedFree(const void* ptr);

  /*! allocates pages directly from OS */
  void* os_malloc (size_t bytes);
  void* os_reserve(size_t bytes);
  void  os_commit (void* ptr, size_t bytes);
  void  os_shrink (void* ptr, size_t bytesNew, size_t bytesOld);
  void  os_free   (void* ptr, size_t bytes);
  void* os_realloc(void* ptr, size_t bytesNew, size_t bytesOld);

  /*! returns performance counter in seconds */
  double getSeconds();
}

#if defined(__MIC__)
#define isa knc
#elif defined (__AVX2__)
#define isa avx2
#elif defined(__AVXI__)
#define isa avxi
#elif defined(__AVX__)
#define isa avx
#elif defined (__SSE4_2__)
#define isa sse42
#elif defined (__SSE4_1__)
#define isa sse41
#elif defined(__SSSE3__)
#define isa ssse3
#elif defined(__SSE3__)
#define isa sse3
#elif defined(__SSE2__)
#define isa sse2
#elif defined(__SSE__)
#define isa sse
#else 
#error Unknown ISA
#endif
