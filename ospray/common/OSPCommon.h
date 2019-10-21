// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
#include "ospcommon/math/AffineSpace.h"
#include "ospcommon/memory/RefCount.h"
#include "ospcommon/memory/malloc.h"

// ospray
#include "ospray/ospray.h"
#include "ospray/ospray_cpp/Traits.h"

// std
#include <cstdint> // for int64_t etc
#include <sstream>

// NOTE(jda) - This one file is shared between the core OSPRay library and the
//             ispc module...thus we have to define 2 different EXPORTS macros
//             here. This needs to be split between the libraries!
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
#define OSPRAY_CORE_INTERFACE OSPRAY_INTERFACE

#ifdef _WIN32
#  ifdef ospray_module_ispc_EXPORTS
#    define OSPRAY_MODULE_ISPC_INTERFACE __declspec(dllexport)
#  else
#    define OSPRAY_MODULE_ISPC_INTERFACE __declspec(dllimport)
#  endif
#  define OSPRAY_MODULE_ISPC_DLLEXPORT __declspec(dllexport)
#else
#  define OSPRAY_MODULE_ISPC_INTERFACE
#  define OSPRAY_MODULE_ISPC_DLLEXPORT
#endif
#define OSPRAY_SDK_INTERFACE OSPRAY_MODULE_ISPC_INTERFACE

#define OSP_REGISTER_OBJECT(Object, object_name, InternalClass, external_name) \
  extern "C" OSPRAY_DLLEXPORT                                                  \
      Object *ospray_create_##object_name##__##external_name()                 \
  {                                                                            \
    return new InternalClass;                                                  \
  }                                                                            \
  /* additional declaration to avoid "extra ;" -Wpedantic warnings */          \
  Object *ospray_create_##object_name##__##external_name()

//! main namespace for all things ospray (for internal code)
namespace ospray {

  using namespace ospcommon;
  using namespace ospcommon::math;
  using namespace ospcommon::memory;

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

  extern "C" {
    /*! 64-bit malloc. allows for alloc'ing memory larger than 4GB */
    OSPRAY_CORE_INTERFACE void *malloc64(size_t size);
    /*! 64-bit malloc. allows for alloc'ing memory larger than 4GB */
    OSPRAY_CORE_INTERFACE void free64(void *ptr);
  }

  OSPRAY_CORE_INTERFACE OSPDataType typeOf(const char *string);
  inline OSPDataType typeOf(const std::string &s)
  {
    return typeOf(s.c_str());
  }

  OSPRAY_CORE_INTERFACE std::string stringFor(OSPDataType);
  OSPRAY_CORE_INTERFACE std::string stringFor(OSPTextureFormat);

  inline bool isObjectType(OSPDataType type)
  {
    return type & OSP_OBJECT;
  }

  OSPRAY_CORE_INTERFACE size_t sizeOf(OSPDataType);
  OSPRAY_CORE_INTERFACE size_t sizeOf(OSPTextureFormat);

  OSPRAY_CORE_INTERFACE OSPError loadLocalModule(const std::string &name);

  /*! little helper class that prints out a warning string upon the
    first time it is encountered.

    Usage:

    if (someThisBadHappens) {
       static WarnOnce warning("something bad happened, at least once!");
       ...
    }
  */
  struct OSPRAY_CORE_INTERFACE WarnOnce
  {
    WarnOnce(const std::string &message, uint32_t postAtLogLevel = 0);
  private:
    const std::string s;
  };

  OSPRAY_CORE_INTERFACE uint32_t logLevel();

  OSPRAY_CORE_INTERFACE void postStatusMsg(const std::stringstream &msg,
                                          uint32_t postAtLogLevel = 0);

  OSPRAY_CORE_INTERFACE void postStatusMsg(const std::string &msg,
                                          uint32_t postAtLogLevel = 0);

  // use postStatusMsg to output any information, which will use OSPRay's
  // internal infrastructure, optionally at a given loglevel
  // a newline is added implicitly
  //////////////////////////////////////////////////////////////////////////////

  struct StatusMsgStream : public std::stringstream
  {
    StatusMsgStream(uint32_t postAtLogLevel = 0);
    StatusMsgStream(StatusMsgStream &&other);
    ~StatusMsgStream();

  private:

    uint32_t logLevel {0};
  };

  inline StatusMsgStream::StatusMsgStream(uint32_t postAtLogLevel)
    : logLevel(postAtLogLevel)
  {
  }

  inline StatusMsgStream::~StatusMsgStream()
  {
    auto msg = str();
    if (!msg.empty())
      postStatusMsg(msg, logLevel);
  }

  inline StatusMsgStream::StatusMsgStream(StatusMsgStream &&other)
  {
    this->str(other.str());
  }

  OSPRAY_CORE_INTERFACE StatusMsgStream postStatusMsg(uint32_t postAtLogLevel = 0);

  /////////////////////////////////////////////////////////////////////////////

  OSPRAY_CORE_INTERFACE void handleError(OSPError e, const std::string &message);

  OSPRAY_CORE_INTERFACE void postTraceMsg(const std::string &message);

  //! log status message at loglevel x
  #define ospLog(x) StatusMsgStream(x) << "(" << x << "): "
  //! log status message at loglevel x with function name
  #define ospLogF(x) StatusMsgStream(x) << __FUNCTION__ << ": "
  //! log status message at loglevel x with function, file, and line number
  #define ospLogL(x) StatusMsgStream(x) << __FUNCTION__ << "(" << __FILE__ << ":" << __LINE__ << "): "

  // RTTI hash ID lookup helper functions ///////////////////////////////////

  OSPRAY_CORE_INTERFACE size_t translatedHash(size_t v);

  template <typename T>
  inline size_t typeIdOf()
  {
    return translatedHash(typeid(T).hash_code());
  }

  template <typename T>
  inline size_t typeIdOf(T *v)
  {
    return translatedHash(typeid(*v).hash_code());
  }

  template <typename T>
  inline size_t typeIdOf(const T &v)
  {
    return translatedHash(typeid(v).hash_code());
  }

  template <typename T>
  inline size_t typeIdOf(const std::unique_ptr<T> &v)
  {
    return translatedHash(typeid(*v).hash_code());
  }

  template <typename T>
  inline size_t typeIdOf(const std::shared_ptr<T> &v)
  {
    return translatedHash(typeid(*v).hash_code());
  }

  // RTTI string name lookup helper functions ///////////////////////////////

  template <typename T>
  inline std::string typeString()
  {
    return typeid(T).name();
  }

  template <typename T>
  inline std::string typeString(T *v)
  {
    return typeid(*v).name();
  }

  template <typename T>
  inline std::string typeString(const T &v)
  {
    return typeid(v).name();
  }

  template <typename T>
  inline std::string typeString(const std::unique_ptr<T> &v)
  {
    return typeid(*v).name();
  }

  template <typename T>
  inline std::string typeString(const std::shared_ptr<T> &v)
  {
    return typeid(*v).name();
  }


#define OSPTYPEFOR_DEFINITION(type) \
  constexpr OSPDataType OSPTypeFor<type>::value

} // ::ospray
