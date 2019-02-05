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

#define CACHELINE_SIZE 64

#if !defined(PAGE_SIZE)
  #define PAGE_SIZE 4096
#endif

#define MAX_THREADS 512

#include "common.h"

namespace ospcommon
{
  enum CPUModel {
    CPU_UNKNOWN,
    CPU_CORE1,
    CPU_CORE2,
    CPU_CORE_NEHALEM,
    CPU_CORE_SANDYBRIDGE,
    CPU_HASWELL,
    CPU_KNC,
    CPU_KNL
  };

  /*! get the full path to the running executable */
  OSPCOMMON_INTERFACE std::string getExecutableFileName();

  /*! return platform name */
  OSPCOMMON_INTERFACE std::string getPlatformName();

  /*! get the full name of the compiler */
  OSPCOMMON_INTERFACE std::string getCompilerName();

  /*! return the name of the CPU */
  OSPCOMMON_INTERFACE std::string getCPUVendor();

  /*! get microprocessor model */
  OSPCOMMON_INTERFACE CPUModel getCPUModel();

  /*! converts CPU model into string */
  OSPCOMMON_INTERFACE std::string stringOfCPUModel(CPUModel model);

  /*! CPU features */
  static const int CPU_FEATURE_SSE    = 1 << 0;
  static const int CPU_FEATURE_SSE2   = 1 << 1;
  static const int CPU_FEATURE_SSE3   = 1 << 2;
  static const int CPU_FEATURE_SSSE3  = 1 << 3;
  static const int CPU_FEATURE_SSE41  = 1 << 4;
  static const int CPU_FEATURE_SSE42  = 1 << 5; 
  static const int CPU_FEATURE_POPCNT = 1 << 6;
  static const int CPU_FEATURE_AVX    = 1 << 7;
  static const int CPU_FEATURE_F16C   = 1 << 8;
  static const int CPU_FEATURE_RDRAND = 1 << 9;
  static const int CPU_FEATURE_AVX2   = 1 << 10;
  static const int CPU_FEATURE_FMA3   = 1 << 11;
  static const int CPU_FEATURE_LZCNT  = 1 << 12;
  static const int CPU_FEATURE_BMI1   = 1 << 13;
  static const int CPU_FEATURE_BMI2   = 1 << 14;
  static const int CPU_FEATURE_KNC    = 1 << 15;
  static const int CPU_FEATURE_AVX512F = 1 << 16;
  static const int CPU_FEATURE_AVX512DQ = 1 << 17;    
  static const int CPU_FEATURE_AVX512PF = 1 << 18;
  static const int CPU_FEATURE_AVX512ER = 1 << 19;
  static const int CPU_FEATURE_AVX512CD = 1 << 20;
  static const int CPU_FEATURE_AVX512BW = 1 << 21;
  static const int CPU_FEATURE_AVX512VL = 1 << 22;
  static const int CPU_FEATURE_AVX512IFMA = 1 << 23;
  static const int CPU_FEATURE_AVX512VBMI = 1 << 24;
 
  /*! get CPU features */
  OSPCOMMON_INTERFACE int getCPUFeatures();

  /*! convert CPU features into a string */
  OSPCOMMON_INTERFACE std::string stringOfCPUFeatures(int features);

  /*! ISAs */
  static const int SSE    = CPU_FEATURE_SSE; 
  static const int SSE2   = SSE | CPU_FEATURE_SSE2;
  static const int SSE3   = SSE2 | CPU_FEATURE_SSE3;
  static const int SSSE3  = SSE3 | CPU_FEATURE_SSSE3;
  static const int SSE41  = SSSE3 | CPU_FEATURE_SSE41;
  static const int SSE42  = SSE41 | CPU_FEATURE_SSE42 | CPU_FEATURE_POPCNT;
  static const int AVX    = SSE42 | CPU_FEATURE_AVX;
  static const int AVXI   = AVX | CPU_FEATURE_F16C | CPU_FEATURE_RDRAND;
  static const int AVX2   = AVXI | CPU_FEATURE_AVX2 | CPU_FEATURE_FMA3 | CPU_FEATURE_BMI1 | CPU_FEATURE_BMI2 | CPU_FEATURE_LZCNT;
  static const int KNC    = CPU_FEATURE_KNC;
  static const int AVX512KNL = AVX2 | CPU_FEATURE_AVX512F | CPU_FEATURE_AVX512PF | CPU_FEATURE_AVX512ER | CPU_FEATURE_AVX512CD;
  static const int AVX512SKX = AVX2 | CPU_FEATURE_AVX512F | CPU_FEATURE_AVX512DQ | CPU_FEATURE_AVX512CD | CPU_FEATURE_AVX512BW | CPU_FEATURE_AVX512VL;

  /*! converts ISA bitvector into a string */
  OSPCOMMON_INTERFACE std::string stringOfISA(int features);

  /*! return the number of logical threads of the system */
  OSPCOMMON_INTERFACE size_t getNumberOfLogicalThreads();
  
  /*! returns the size of the terminal window in characters */
  OSPCOMMON_INTERFACE int getTerminalWidth();

  /*! returns performance counter in seconds */
  OSPCOMMON_INTERFACE double getSeconds();
}
