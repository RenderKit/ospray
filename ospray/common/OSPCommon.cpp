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

#include "OSPCommon.h"
#ifdef OSPRAY_USE_INTERNAL_TASKING
#  include "ospray/common/tasking/TaskSys.h"
#endif
#include "ospray/common/tasking/async.h"
// embree
#include "embree2/rtcore.h"
#include "common/sysinfo.h"
//stl
#include <thread>

namespace ospray {

  RTCDevice g_embreeDevice = NULL;

  /*! 64-bit malloc. allows for alloc'ing memory larger than 64 bits */
  extern "C" void *malloc64(size_t size)
  {
    return ospcommon::alignedMalloc(size);
  }

  /*! 64-bit malloc. allows for alloc'ing memory larger than 64 bits */
  extern "C" void free64(void *ptr)
  {
    return ospcommon::alignedFree(ptr);
  }

  /*! logging level - '0' means 'no logging at all', increasing
      numbers mean increasing verbosity of log messages */
  uint32_t logLevel = 0;
  bool debugMode = false;
  int32_t numThreads = -1; //!< for default (==maximum) number of
                           //   OSPRay/Embree threads

  WarnOnce::WarnOnce(const std::string &s) 
    : s(s) 
  {
    std::cout << "Warning: " << s << " (only reporting first occurrence)"
              << std::endl;
  }
  
  /*! for debugging. compute a checksum for given area range... */
  void *computeCheckSum(const void *ptr, size_t numBytes)
  {
    long *end = (long *)((char *)ptr + (numBytes - (numBytes%8)));
    long *mem = (long *)ptr;
    long sum = 0;
    long i = 0;

    while (mem < end) {
      sum += (i+13) * *mem;
      ++i;
      ++mem;
    }
    return (void *)sum;
  }

  void doAssertion(const char *file, int line,
                   const char *expr, const char *expl) {
    if (expl)
      fprintf(stderr,"%s:%i: Assertion failed: \"%s\":\nAdditional Info: %s\n", 
              file, line, expr, expl);
    else
      fprintf(stderr,"%s:%i: Assertion failed: \"%s\".\n", file, line, expr);
    abort();
  }

  void removeArgs(int &ac, char **&av, int where, int howMany)
  {
    for (int i=where+howMany;i<ac;i++)
      av[i-howMany] = av[i];
    ac -= howMany;
  }

  void init(int *_ac, const char ***_av)
  {
#ifndef OSPRAY_TARGET_MIC
    // If we're not on a MIC, check for SSE4.1 as minimum supported ISA. Will
    // be increased to SSE4.2 in future.
    int cpuFeatures = ospcommon::getCPUFeatures();
    if ((cpuFeatures & ospcommon::CPU_FEATURE_SSE41) == 0)
      throw std::runtime_error("Error. OSPRay only runs on CPUs that support"
                               " at least SSE4.1.");
#endif

    if (_ac && _av) {
      int &ac = *_ac;
      char ** &av = *(char ***)_av;
      for (int i=1;i<ac;) {
        std::string parm = av[i];
        if (parm == "--osp:debug") {
          debugMode = true;
          numThreads = 1;
          removeArgs(ac,av,i,1);
        } else if (parm == "--osp:verbose") {
          logLevel = 1;
          removeArgs(ac,av,i,1);
        } else if (parm == "--osp:vv") {
          logLevel = 2;
          removeArgs(ac,av,i,1);
        } else if (parm == "--osp:loglevel") {
          logLevel = atoi(av[i+1]);
          removeArgs(ac,av,i,2);
        } else if (parm == "--osp:numthreads" || parm == "--osp:num-threads") {
          numThreads = atoi(av[i+1]);
          removeArgs(ac,av,i,2);
        } else {
          ++i;
        }
      }
    }

#ifdef OSPRAY_TASKING_INTERNAL
    try {
      ospray::Task::initTaskSystem(debugMode ? 0 : numThreads);
    } catch (const std::runtime_error &e) {
      std::cerr << "WARNING: " << e.what() << std::endl;
    }
#endif
  }

  void error_handler(const RTCError code, const char *str)
  {
    printf("Embree: ");
    switch (code) {
      case RTC_UNKNOWN_ERROR    : printf("RTC_UNKNOWN_ERROR"); break;
      case RTC_INVALID_ARGUMENT : printf("RTC_INVALID_ARGUMENT"); break;
      case RTC_INVALID_OPERATION: printf("RTC_INVALID_OPERATION"); break;
      case RTC_OUT_OF_MEMORY    : printf("RTC_OUT_OF_MEMORY"); break;
      case RTC_UNSUPPORTED_CPU  : printf("RTC_UNSUPPORTED_CPU"); break;
      default                   : printf("invalid error code"); break;
    }
    if (str) { 
      printf(" ("); 
      while (*str) putchar(*str++); 
      printf(")\n"); 
    }
    abort();
  }

  size_t sizeOf(const OSPDataType type) {
    switch (type) {
    case OSP_VOID_PTR:  return sizeof(void *);
    case OSP_OBJECT:    return sizeof(void *);
    case OSP_DATA:      return sizeof(void *);
    case OSP_CHAR:      return sizeof(int8);
    case OSP_UCHAR:     return sizeof(uint8);
    case OSP_UCHAR2:    return sizeof(vec2uc);
    case OSP_UCHAR3:    return sizeof(vec3uc);
    case OSP_UCHAR4:    return sizeof(vec4uc);
    case OSP_USHORT:    return sizeof(uint16);
    case OSP_INT:       return sizeof(int32);
    case OSP_INT2:      return sizeof(vec2i);
    case OSP_INT3:      return sizeof(vec3i);
    case OSP_INT4:      return sizeof(vec4i);
    case OSP_UINT:      return sizeof(uint32);
    case OSP_UINT2:     return sizeof(vec2ui);
    case OSP_UINT3:     return sizeof(vec3ui);
    case OSP_UINT4:     return sizeof(vec4ui);
    case OSP_LONG:      return sizeof(int64);
    case OSP_LONG2:     return sizeof(vec2l);
    case OSP_LONG3:     return sizeof(vec3l);
    case OSP_LONG4:     return sizeof(vec4l);
    case OSP_ULONG:     return sizeof(uint64);
    case OSP_ULONG2:    return sizeof(vec2ul);
    case OSP_ULONG3:    return sizeof(vec3ul);
    case OSP_ULONG4:    return sizeof(vec4ul);
    case OSP_FLOAT:     return sizeof(float);
    case OSP_FLOAT2:    return sizeof(vec2f);
    case OSP_FLOAT3:    return sizeof(vec3f);
    case OSP_FLOAT4:    return sizeof(vec4f);
    case OSP_FLOAT3A:   return sizeof(vec3fa);
    case OSP_DOUBLE:    return sizeof(double);
    default: break;
    };

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown OSPDataType "
          << (int)type;
    throw std::runtime_error(error.str());
  }

  OSPDataType typeForString(const char *string) {
    if (string == NULL)                return(OSP_UNKNOWN);
    if (strcmp(string, "char"  ) == 0) return(OSP_CHAR);
    if (strcmp(string, "double") == 0) return(OSP_DOUBLE);
    if (strcmp(string, "float" ) == 0) return(OSP_FLOAT);
    if (strcmp(string, "float2") == 0) return(OSP_FLOAT2);
    if (strcmp(string, "float3") == 0) return(OSP_FLOAT2);
    if (strcmp(string, "float4") == 0) return(OSP_FLOAT2);
    if (strcmp(string, "int"   ) == 0) return(OSP_INT);
    if (strcmp(string, "int2"  ) == 0) return(OSP_INT2);
    if (strcmp(string, "int3"  ) == 0) return(OSP_INT3);
    if (strcmp(string, "int4"  ) == 0) return(OSP_INT4);
    if (strcmp(string, "uchar" ) == 0) return(OSP_UCHAR);
    if (strcmp(string, "uchar2") == 0) return(OSP_UCHAR2);
    if (strcmp(string, "uchar3") == 0) return(OSP_UCHAR3);
    if (strcmp(string, "uchar4") == 0) return(OSP_UCHAR4);
    if (strcmp(string, "ushort") == 0) return(OSP_USHORT);
    if (strcmp(string, "uint"  ) == 0) return(OSP_UINT);
    if (strcmp(string, "uint2" ) == 0) return(OSP_UINT2);
    if (strcmp(string, "uint3" ) == 0) return(OSP_UINT3);
    if (strcmp(string, "uint4" ) == 0) return(OSP_UINT4);
    return(OSP_UNKNOWN);
  }

  size_t sizeOf(const OSPTextureFormat type) {
    switch (type) {
      case OSP_TEXTURE_RGBA8:
      case OSP_TEXTURE_SRGBA:   return sizeof(uint32);
      case OSP_TEXTURE_RGBA32F: return sizeof(vec4f);
      case OSP_TEXTURE_RGB8:
      case OSP_TEXTURE_SRGB:    return sizeof(vec3uc);
      case OSP_TEXTURE_RGB32F:  return sizeof(vec3f);
      case OSP_TEXTURE_R8:      return sizeof(uint8);
      case OSP_TEXTURE_R32F:    return sizeof(float);
    }

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown OSPTextureFormat "
          << (int)type;
    throw std::runtime_error(error.str());
  }

} // ::ospray

