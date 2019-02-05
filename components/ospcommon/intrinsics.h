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

#include "platform.h"

/* normally defined in pmmintrin.h, but we always need this */
#if !defined(_MM_SET_DENORMALS_ZERO_MODE)
#define _MM_DENORMALS_ZERO_ON   (0x0040)
#define _MM_DENORMALS_ZERO_OFF  (0x0000)
#define _MM_DENORMALS_ZERO_MASK (0x0040)
#define _MM_SET_DENORMALS_ZERO_MODE(x) (_mm_setcsr((_mm_getcsr() & ~_MM_DENORMALS_ZERO_MASK) | (x)))
#endif

namespace ospcommon
{
  
////////////////////////////////////////////////////////////////////////////////
/// Windows Platform
////////////////////////////////////////////////////////////////////////////////
  
#ifdef _WIN32
  
  __forceinline size_t read_tsc()  
  {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (size_t)li.QuadPart;
  }
  
////////////////////////////////////////////////////////////////////////////////
/// Unix Platform
////////////////////////////////////////////////////////////////////////////////
  
#else
  
#if defined(__i386__) && defined(__PIC__)
  
  __forceinline void __cpuid(int out[4], int op) 
  {
    asm volatile ("xchg{l}\t{%%}ebx, %1\n\t"
                  "cpuid\n\t"
                  "xchg{l}\t{%%}ebx, %1\n\t"
                  : "=a"(out[0]), "=r"(out[1]), "=c"(out[2]), "=d"(out[3]) 
                  : "0"(op)); 
  }
  
  __forceinline void __cpuid_count(int out[4], int op1, int op2) 
  {
    asm volatile ("xchg{l}\t{%%}ebx, %1\n\t"
                  "cpuid\n\t"
                  "xchg{l}\t{%%}ebx, %1\n\t"
                  : "=a" (out[0]), "=r" (out[1]), "=c" (out[2]), "=d" (out[3])
                  : "0" (op1), "2" (op2)); 
  }
  
#else
  
  __forceinline void __cpuid(int out[4], int op) {
    asm volatile ("cpuid" : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3]) : "a"(op)); 
  }
  
  __forceinline void __cpuid_count(int out[4], int op1, int op2) {
    asm volatile ("cpuid" : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3]) : "a"(op1), "c"(op2)); 
  }
  
#endif
  
  __forceinline uint64_t read_tsc()  {
    uint32_t high,low;
    asm volatile ("rdtsc" : "=d"(high), "=a"(low));
    return (((uint64_t)high) << 32) + (uint64_t)low;
  }
  
#endif 
  
////////////////////////////////////////////////////////////////////////////////
/// All Platforms
////////////////////////////////////////////////////////////////////////////////
  
  
  __forceinline uint64_t rdtsc()
  {
    int dummy[4]; 
    __cpuid(dummy,0); 
    uint64_t clock = read_tsc(); 
    __cpuid(dummy,0); 
    return clock;
  }
  
}
