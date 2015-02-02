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

#include "OSPCommon.h"
// embree
#include "embree2/rtcore.h"

// std
#include <time.h>
#ifdef __WIN32__
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h> // for GetSystemTime
#else
#include <sys/time.h>
#include <sys/times.h>
#endif

namespace ospray {

  /*! logging level - '0' means 'no logging at all', increasing
      numbers mean increasing verbosity of log messages */
  uint32 logLevel = 0;
  bool debugMode = false;

  /*! for debugging. compute a checksum for given area range... */
  void *computeCheckSum(const void *ptr, size_t numBytes)
  {
    long *end = (long *)((char *)ptr + (numBytes - (numBytes%8)));
    long *mem = (long *)ptr;
    long sum = 0;
    long i = 0;

    int nextPing = 1;
    while (mem < end) {
      sum += (i+13) * *mem;
      ++i;
      ++mem;
      
      // if (i==nextPing) { 
      //  std::cout << "checksum after " << (i*8) << " bytes: " << (int*)sum << std::endl;
      //   nextPing += nextPing;
     // }
    }
    return (void *)sum;
  }

  void doAssertion(const char *file, int line, const char *expr, const char *expl) {
    if (expl)
      fprintf(stderr,"%s:%i: Assertion failed: \"%s\":\nAdditional Info: %s\n", 
              file, line, expr, expl);
    else
      fprintf(stderr,"%s:%i: Assertion failed: \"%s\".\n", file, line, expr);
    abort();
  }

  double getSysTime() {
#ifdef __WIN32__
    SYSTEMTIME tp; GetSystemTime(&tp);
    return double(tp.wSecond) + double(tp.wMilliseconds) / 1E3;
#else
    struct timeval tp; gettimeofday(&tp,NULL); 
    return double(tp.tv_sec) + double(tp.tv_usec)/1E6; 
#endif
  }

  void init(int *_ac, const char ***_av)
  {
    int &ac = *_ac;
    char ** &av = *(char ***)_av;
    for (int i=1;i<ac;) {
      std::string parm = av[i];
      if (parm == "--osp:debug") {
        debugMode = true;
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
      } else {
        ++i;
      }
    }
  }

  void removeArgs(int &ac, char **&av, int where, int howMany)
  {
    for (int i=where+howMany;i<ac;i++)
      av[i-howMany] = av[i];
    ac -= howMany;
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

  size_t sizeOf(OSPDataType type) {

    switch (type) {
    case OSP_VOID_PTR:  return sizeof(void *);
    case OSP_OBJECT:    return sizeof(void *);
    case OSP_CHAR:      return sizeof(int8);
    case OSP_UCHAR:     return sizeof(uint8);
    case OSP_UCHAR2:    return sizeof(embree::Vec2<uint8>);
    case OSP_UCHAR3:    return sizeof(embree::Vec3<uint8>);
    case OSP_UCHAR4:    return sizeof(uint32);
    case OSP_INT:       return sizeof(int32);
    case OSP_INT2:      return sizeof(embree::Vec2<int32>);
    case OSP_INT3:      return sizeof(embree::Vec3<int32>);
    case OSP_INT4:      return sizeof(embree::Vec4<int32>);
    case OSP_UINT:      return sizeof(uint32);
    case OSP_UINT2:     return sizeof(embree::Vec2<uint32>);
    case OSP_UINT3:     return sizeof(embree::Vec3<uint32>);
    case OSP_UINT4:     return sizeof(embree::Vec4<uint32>);
    case OSP_FLOAT:     return sizeof(float);
    case OSP_FLOAT2:    return sizeof(embree::Vec2<float>);
    case OSP_FLOAT3:    return sizeof(embree::Vec3<float>);
    case OSP_FLOAT4:    return sizeof(embree::Vec4<float>);
    case OSP_FLOAT3A:   return sizeof(embree::Vec3fa);
    default: break;
    };

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown OSPDataType " << (int)type;
    throw std::runtime_error(error.str());

  }

} // ::ospray

