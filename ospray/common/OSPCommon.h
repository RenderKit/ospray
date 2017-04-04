// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

/*! \file OSPCommon.h Defines common types and classes that _every_
  ospray file should know about */

// include cmake config first
#include "OSPConfig.h"

#ifdef _WIN32
// ----------- windows only -----------
typedef unsigned long long id_t;
# ifndef _USE_MATH_DEFINES
#   define _USE_MATH_DEFINES
# endif
# include <cmath>
# include <math.h>
# ifdef _M_X64
typedef long long ssize_t;
# else
typedef int ssize_t;
# endif
#else
// ----------- NOT windows -----------
# include "unistd.h"
#endif

// ospcommon
#include "ospcommon/AffineSpace.h"
#include "ospcommon/RefCount.h"
#include "ospcommon/malloc.h"

#if 0
// stl
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <type_traits>
#endif

// ospray
#include "ospray/OSPDataType.h"
#include "ospray/OSPTexture.h"

// std
#include <cstdint> // for int64_t etc
#include <sstream>

#ifdef _WIN32
#  ifdef ospray_EXPORTS
#    define OSPRAY_INTERFACE __declspec(dllexport)
#  else
#    define OSPRAY_INTERFACE __declspec(dllimport)
#  endif
#  define OSPRAY_DLLEXPORT __declspec(dllexport)
#else
#  define OSPRAY_INTERFACE
#  define OSPRAY_DLLEXPORT
#endif
#define OSPRAY_SDK_INTERFACE OSPRAY_INTERFACE

//! main namespace for all things ospray (for internal code)
namespace ospray {

  using namespace ospcommon;

  /*! basic types */
  using int64  = std::int64_t;
  using uint64 = std::uint64_t;

  using int32  = std::int32_t;
  using uint32 = std::uint32_t;

  using int16  = std::int16_t;
  using uint16 = std::uint16_t;

  using int8  = std::int8_t;
  using uint8 = std::uint8_t;

  using index_t = std::int64_t;

  void initFromCommandLine(int *ac = nullptr, const char ***av = nullptr);

  /*! for debugging. compute a checksum for given area range... */
  OSPRAY_INTERFACE void *computeCheckSum(const void *ptr, size_t numBytes);

#ifndef Assert
#ifdef NDEBUG
# define Assert(expr) /* nothing */
# define Assert2(expr,expl) /* nothing */
# define AssertError(errMsg) /* nothing */
#else
# define Assert(expr)                                                   \
  ((void)((expr) ? 0 : ((void)ospray::doAssertion(__FILE__, __LINE__, #expr, NULL), 0)))
# define Assert2(expr,expl)                                             \
  ((void)((expr) ? 0 : ((void)ospray::doAssertion(__FILE__, __LINE__, #expr, expl), 0)))
# define AssertError(errMsg)                            \
  doAssertion(__FILE__,__LINE__, (errMsg), NULL)
#endif
#endif

  /*! size of OSPDataType */
  OSPRAY_INTERFACE size_t sizeOf(const OSPDataType);

  /*! Convert a type string to an OSPDataType. */
  OSPRAY_INTERFACE OSPDataType typeForString(const char *string);
  /*! Convert a type string to an OSPDataType. */
  inline OSPDataType typeForString(const std::string &s)
  { return typeForString(s.c_str()); }

  /*! Convert a type string to an OSPDataType. */
  OSPRAY_INTERFACE std::string stringForType(OSPDataType type);

  /*! size of OSPTextureFormat */
  OSPRAY_INTERFACE size_t sizeOf(const OSPTextureFormat);


  /*! little helper class that prints out a warning string upon the
    first time it is encountered.

    Usage:

    if (someThisBadHappens) {
       static WarnOnce warning("something bad happened, at least once!");
       ...
    }
  */
  struct OSPRAY_SDK_INTERFACE WarnOnce
  {
    WarnOnce(const std::string &warning, uint32_t postAtLogLevel = 0);
  private:
    const std::string s;
  };

  OSPRAY_INTERFACE uint32_t logLevel();

  OSPRAY_INTERFACE int loadLocalModule(const std::string &name);

  OSPRAY_INTERFACE void postErrorMsg(const std::stringstream &msg,
                                     uint32_t postAtLogLevel = 0);

  OSPRAY_INTERFACE void postErrorMsg(const std::string &msg,
                                     uint32_t postAtLogLevel = 0);

} // ::ospray

#ifdef _WIN32
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif
#define NOTIMPLEMENTED    throw std::runtime_error(std::string(__PRETTY_FUNCTION__)+": not implemented...");

