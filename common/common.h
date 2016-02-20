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

#pragma once

#include "platform.h"
// c runtime
#include <math.h>
// std
#include <stdexcept>

#ifdef _WIN32
  typedef unsigned long long id_t;
#endif

#if defined(__WIN32__) || defined(_WIN32)
// ----------- windows only -----------
# define _USE_MATH_DEFINES 1
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

#include <stdint.h>

#ifdef _WIN32
#  ifdef ospcommon_EXPORTS
#    define OSPCOMMON_INTERFACE __declspec(dllexport)
#  else
#    define OSPCOMMON_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OSPCOMMON_INTERFACE
#endif

#ifdef NDEBUG
# define Assert(expr) /* nothing */
# define Assert2(expr,expl) /* nothing */
# define AssertError(errMsg) /* nothing */
#else
# define Assert(expr)                                                   \
  ((void)((expr) ? 0 : ((void)ospcommon::doAssertion(__FILE__, __LINE__, #expr, NULL), 0)))
# define Assert2(expr,expl)                                             \
  ((void)((expr) ? 0 : ((void)ospcommon::doAssertion(__FILE__, __LINE__, #expr, expl), 0)))
# define AssertError(errMsg)                            \
  doAssertion(__FILE__,__LINE__, (errMsg), NULL)
#endif

#ifdef _WIN32
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif
#define NOTIMPLEMENTED    throw std::runtime_error(std::string(__PRETTY_FUNCTION__)+": not implemented...");

namespace ospcommon {

  /*! return system time in seconds */
  OSPCOMMON_INTERFACE double getSysTime();
  OSPCOMMON_INTERFACE void doAssertion(const char *file, int line, const char *expr, const char *expl);
  /*! remove specified num arguments from an ac/av arglist */
  OSPCOMMON_INTERFACE void removeArgs(int &ac, char **&av, int where, int howMany);
  OSPCOMMON_INTERFACE void  loadLibrary(const std::string &_name);
  OSPCOMMON_INTERFACE void *getSymbol(const std::string &name);

} // ::ospcommon
