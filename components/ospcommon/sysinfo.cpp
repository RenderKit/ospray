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

#include "sysinfo.h"
//#include "string.h"
#include "intrinsics.h"
#include <sstream>

////////////////////////////////////////////////////////////////////////////////
/// All Platforms
////////////////////////////////////////////////////////////////////////////////

namespace ospcommon
{
  std::string to_string(long long l)
  {
    std::stringstream ss;
    ss << l;
    return ss.str();
  }

  std::string getCompilerName()
  {
#if defined(__INTEL_COMPILER)
    int icc_mayor = __INTEL_COMPILER / 100 % 100;
    int icc_minor = __INTEL_COMPILER % 100;
    std::string version = "Intel Compiler ";
    version += ospcommon::to_string((long long)icc_mayor);
    version += "." + ospcommon::to_string((long long)icc_minor);
#if defined(__INTEL_COMPILER_UPDATE)
    version += "." + ospcommon::to_string((long long)__INTEL_COMPILER_UPDATE);
#endif
    return version;
#elif defined(__clang__)
    return "CLANG " __clang_version__;
#elif defined (__GNUC__)
    return "GCC " __VERSION__;
#elif defined(_MSC_VER)
    std::string version = ospcommon::to_string((long long)_MSC_FULL_VER);
    version.insert(4,".");
    version.insert(9,".");
    version.insert(2,".");
    return "Visual C++ Compiler " + version;
#else
    return "Unknown Compiler";
#endif
  }

  std::string getCPUVendor()
  {
    int cpuinfo[4];
    __cpuid (cpuinfo, 0);
    int name[4];
    name[0] = cpuinfo[1];
    name[1] = cpuinfo[3];
    name[2] = cpuinfo[2];
    name[3] = 0;
    return (char*)name;
  }

  CPUModel getCPUModel()
  {
    int out[4];
    __cpuid(out, 0);
    if (out[0] < 1) return CPU_UNKNOWN;
    __cpuid(out, 1);
    int family = ((out[0] >> 8) & 0x0F) + ((out[0] >> 20) & 0xFF);
    int model  = ((out[0] >> 4) & 0x0F) | ((out[0] >> 12) & 0xF0);
    if (family ==  11) return CPU_KNC;
    if (family !=   6) return CPU_UNKNOWN;           // earlier than P6
    if (model == 0x0E) return CPU_CORE1;             // Core 1
    if (model == 0x0F) return CPU_CORE2;             // Core 2, 65 nm
    if (model == 0x16) return CPU_CORE2;             // Core 2, 65 nm Celeron
    if (model == 0x17) return CPU_CORE2;             // Core 2, 45 nm
    if (model == 0x1A) return CPU_CORE_NEHALEM;      // Core i7, Nehalem
    if (model == 0x1E) return CPU_CORE_NEHALEM;      // Core i7
    if (model == 0x1F) return CPU_CORE_NEHALEM;      // Core i7
    if (model == 0x2C) return CPU_CORE_NEHALEM;      // Core i7, Xeon
    if (model == 0x2E) return CPU_CORE_NEHALEM;      // Core i7, Xeon
    if (model == 0x2A) return CPU_CORE_SANDYBRIDGE;  // Core i7, SandyBridge
    if (model == 0x2D) return CPU_CORE_SANDYBRIDGE;  // Core i7, SandyBridge
    if (model == 0x45) return CPU_HASWELL;           // Haswell
    if (model == 0x3C) return CPU_HASWELL;           // Haswell
    return CPU_UNKNOWN;
  }

  std::string stringOfCPUModel(CPUModel model)
  {
    switch (model) {
    case CPU_KNC             : return "Knights Corner";
    case CPU_CORE1           : return "Core1";
    case CPU_CORE2           : return "Core2";
    case CPU_CORE_NEHALEM    : return "Nehalem";
    case CPU_CORE_SANDYBRIDGE: return "SandyBridge";
    case CPU_HASWELL         : return "Haswell";
    case CPU_KNL             : return "Knights Landing";
    case CPU_UNKNOWN         : return "Unknown CPU";
    }

    return "Unknown CPU";
  }

  /* constants to access destination registers of CPUID instruction */
  static const int EAX = 0;
  static const int EBX = 1;
  static const int ECX = 2;
  static const int EDX = 3;

  /* cpuid[eax=0].ecx */
  static const int CPU_FEATURE_BIT_SSE3   = 1 << 0;
  static const int CPU_FEATURE_BIT_SSSE3  = 1 << 9;
  static const int CPU_FEATURE_BIT_FMA3   = 1 << 12;
  static const int CPU_FEATURE_BIT_SSE4_1 = 1 << 19;
  static const int CPU_FEATURE_BIT_SSE4_2 = 1 << 20;
  static const int CPU_FEATURE_BIT_MOVBE  = 1 << 22;
  static const int CPU_FEATURE_BIT_POPCNT = 1 << 23;
  static const int CPU_FEATURE_BIT_XSAVE  = 1 << 26;
  static const int CPU_FEATURE_BIT_OXSAVE = 1 << 27;
  static const int CPU_FEATURE_BIT_AVX    = 1 << 28;
  static const int CPU_FEATURE_BIT_F16C   = 1 << 29;
  static const int CPU_FEATURE_BIT_RDRAND = 1 << 30;

  /* cpuid[eax=1].edx */
  static const int CPU_FEATURE_BIT_SSE  = 1 << 25;
  static const int CPU_FEATURE_BIT_SSE2 = 1 << 26;

  /* cpuid[eax=0x80000001].ecx */
  static const int CPU_FEATURE_BIT_LZCNT = 1 << 5;

  /* cpuid[eax=7,ecx=0].ebx */
  static const int CPU_FEATURE_BIT_BMI1    = 1 << 3;
  static const int CPU_FEATURE_BIT_AVX2    = 1 << 5;
  static const int CPU_FEATURE_BIT_BMI2    = 1 << 8;
  static const int CPU_FEATURE_BIT_AVX512F = 1 << 16;     // AVX512F (foundation)
  static const int CPU_FEATURE_BIT_AVX512DQ = 1 << 17;    // AVX512DQ
  static const int CPU_FEATURE_BIT_AVX512PF = 1 << 26;    // AVX512PF (prefetch)
  static const int CPU_FEATURE_BIT_AVX512ER = 1 << 27;    // AVX512ER (exponential and reciprocal)
  static const int CPU_FEATURE_BIT_AVX512CD = 1 << 28;    // AVX512CD (conflict detection)
  static const int CPU_FEATURE_BIT_AVX512BW = 1 << 30;    // AVX512BW
  static const int CPU_FEATURE_BIT_AVX512VL = 1 << 31;    // AVX512VL (EVEX.128 and EVEX.256 AVX512 instructions)
  static const int CPU_FEATURE_BIT_AVX512IFMA = 1 << 21;  // AVX512IFMA

  /* cpuid[eax=7,ecx=0].ecx */
  static const int CPU_FEATURE_BIT_AVX512VBMI = 1 << 1;   // AVX512VBMI

  __noinline int64_t get_xcr0()
  {
#ifdef _WIN32
    int64_t xcr0 = 0; // int64_t is workaround for compiler bug under VS2013, Win32
#if defined(__INTEL_COMPILER)
    xcr0 = _xgetbv(0);
#elif (defined(_MSC_VER) && (_MSC_FULL_VER >= 160040219)) // min VS2010 SP1 compiler is required
    xcr0 = _xgetbv(0);
#else
#pragma message ("WARNING: AVX not supported by your compiler.")
    xcr0 = 0;
#endif
    return xcr0;

#else

    int xcr0 = 0;
#if defined(__INTEL_COMPILER)
    __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
#elif ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)) && (!defined(__MACOSX__) || defined(__TARGET_AVX__) || defined(__TARGET_AVX2__))
    __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
#elif ((__clang_major__ > 3) || (__clang_major__ == 3 && __clang_minor__ >= 1))
    __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
#else
#pragma message ("WARNING: AVX not supported by your compiler.")
    xcr0 = 0;
#endif
    return xcr0;
#endif
  }

  int getCPUFeatures()
  {
    /* cache CPU features access */
    static int cpu_features = 0;
    if (cpu_features)
      return cpu_features;

    /* get number of CPUID leaves */
    int cpuid_leaf0[4];
    __cpuid(cpuid_leaf0, 0x00000000);
    unsigned nIds = cpuid_leaf0[EAX];

    /* get number of extended CPUID leaves */
    int cpuid_leafe[4];
    __cpuid(cpuid_leafe, 0x80000000);
    unsigned nExIds = cpuid_leafe[EAX];

    /* get CPUID leaves for EAX = 1,7, and 0x80000001 */
    int cpuid_leaf_1[4] = { 0,0,0,0 };
    int cpuid_leaf_7[4] = { 0,0,0,0 };
    int cpuid_leaf_e1[4] = { 0,0,0,0 };
    if (nIds >= 1) __cpuid (cpuid_leaf_1,0x00000001);
#ifdef _WIN32
#if _MSC_VER && (_MSC_FULL_VER < 160040219)
#else
    if (nIds >= 7) __cpuidex(cpuid_leaf_7,0x00000007,0);
#endif
#else
    if (nIds >= 7) __cpuid_count(cpuid_leaf_7,0x00000007,0);
#endif
    if (nExIds >= 0x80000001) __cpuid(cpuid_leaf_e1,0x80000001);

    /* detect if OS saves XMM, YMM, and ZMM states */
    bool xmm_enabled = true;
    bool ymm_enabled = false;
    bool zmm_enabled = false;
    if (cpuid_leaf_1[ECX] & CPU_FEATURE_BIT_OXSAVE) {
      int64_t xcr0 = get_xcr0();
      xmm_enabled = ((xcr0 & 0x02) == 0x02); /* check if xmm are enabled in XCR0 */
      ymm_enabled = xmm_enabled && ((xcr0 & 0x04) == 0x04); /* check if ymm state are enabled in XCR0 */
      zmm_enabled = ymm_enabled && ((xcr0 & 0xE0) == 0xE0); /* check if OPMASK state, upper 256-bit of ZMM0-ZMM15 and ZMM16-ZMM31 state are enabled in XCR0 */
    }

    if (xmm_enabled && cpuid_leaf_1[EDX] & CPU_FEATURE_BIT_SSE   ) cpu_features |= CPU_FEATURE_SSE;
    if (xmm_enabled && cpuid_leaf_1[EDX] & CPU_FEATURE_BIT_SSE2  ) cpu_features |= CPU_FEATURE_SSE2;
    if (xmm_enabled && cpuid_leaf_1[ECX] & CPU_FEATURE_BIT_SSE3  ) cpu_features |= CPU_FEATURE_SSE3;
    if (xmm_enabled && cpuid_leaf_1[ECX] & CPU_FEATURE_BIT_SSSE3 ) cpu_features |= CPU_FEATURE_SSSE3;
    if (xmm_enabled && cpuid_leaf_1[ECX] & CPU_FEATURE_BIT_SSE4_1) cpu_features |= CPU_FEATURE_SSE41;
    if (xmm_enabled && cpuid_leaf_1[ECX] & CPU_FEATURE_BIT_SSE4_2) cpu_features |= CPU_FEATURE_SSE42;
    if (               cpuid_leaf_1[ECX] & CPU_FEATURE_BIT_POPCNT) cpu_features |= CPU_FEATURE_POPCNT;
    if (ymm_enabled && cpuid_leaf_1[ECX] & CPU_FEATURE_BIT_AVX   ) cpu_features |= CPU_FEATURE_AVX;
    if (xmm_enabled && cpuid_leaf_1[ECX] & CPU_FEATURE_BIT_F16C  ) cpu_features |= CPU_FEATURE_F16C;
    if (               cpuid_leaf_1[ECX] & CPU_FEATURE_BIT_RDRAND) cpu_features |= CPU_FEATURE_RDRAND;
    if (ymm_enabled && cpuid_leaf_7[EBX] & CPU_FEATURE_BIT_AVX2  ) cpu_features |= CPU_FEATURE_AVX2;
    if (ymm_enabled && cpuid_leaf_1[ECX] & CPU_FEATURE_BIT_FMA3  ) cpu_features |= CPU_FEATURE_FMA3;
    if (cpuid_leaf_e1[ECX] & CPU_FEATURE_BIT_LZCNT) cpu_features |= CPU_FEATURE_LZCNT;
    if (cpuid_leaf_7 [EBX] & CPU_FEATURE_BIT_BMI1 ) cpu_features |= CPU_FEATURE_BMI1;
    if (cpuid_leaf_7 [EBX] & CPU_FEATURE_BIT_BMI2 ) cpu_features |= CPU_FEATURE_BMI2;

    if (zmm_enabled && cpuid_leaf_7[EBX] & CPU_FEATURE_BIT_AVX512F )  cpu_features |= CPU_FEATURE_AVX512F;
    if (zmm_enabled && cpuid_leaf_7[EBX] & CPU_FEATURE_BIT_AVX512DQ) cpu_features |= CPU_FEATURE_AVX512DQ;
    if (zmm_enabled && cpuid_leaf_7[EBX] & CPU_FEATURE_BIT_AVX512PF) cpu_features |= CPU_FEATURE_AVX512PF;
    if (zmm_enabled && cpuid_leaf_7[EBX] & CPU_FEATURE_BIT_AVX512ER) cpu_features |= CPU_FEATURE_AVX512ER;
    if (zmm_enabled && cpuid_leaf_7[EBX] & CPU_FEATURE_BIT_AVX512CD) cpu_features |= CPU_FEATURE_AVX512CD;
    if (zmm_enabled && cpuid_leaf_7[EBX] & CPU_FEATURE_BIT_AVX512BW) cpu_features |= CPU_FEATURE_AVX512BW;
    if (zmm_enabled && cpuid_leaf_7[EBX] & CPU_FEATURE_BIT_AVX512IFMA) cpu_features |= CPU_FEATURE_AVX512IFMA;
    if (ymm_enabled && cpuid_leaf_7[EBX] & CPU_FEATURE_BIT_AVX512VL) cpu_features |= CPU_FEATURE_AVX512VL; // on purpose ymm_enabled!
    if (zmm_enabled && cpuid_leaf_7[ECX] & CPU_FEATURE_BIT_AVX512VBMI) cpu_features |= CPU_FEATURE_AVX512VBMI;

    return cpu_features;
  }

  std::string stringOfCPUFeatures(int features)
  {
    std::string str;
    if (features & CPU_FEATURE_SSE   ) str += "SSE ";
    if (features & CPU_FEATURE_SSE2  ) str += "SSE2 ";
    if (features & CPU_FEATURE_SSE3  ) str += "SSE3 ";
    if (features & CPU_FEATURE_SSSE3 ) str += "SSSE3 ";
    if (features & CPU_FEATURE_SSE41 ) str += "SSE41 ";
    if (features & CPU_FEATURE_SSE42 ) str += "SSE42 ";
    if (features & CPU_FEATURE_POPCNT) str += "POPCNT ";
    if (features & CPU_FEATURE_AVX   ) str += "AVX ";
    if (features & CPU_FEATURE_F16C  ) str += "F16C ";
    if (features & CPU_FEATURE_RDRAND) str += "RDRAND ";
    if (features & CPU_FEATURE_AVX2  ) str += "AVX2 ";
    if (features & CPU_FEATURE_FMA3  ) str += "FMA3 ";
    if (features & CPU_FEATURE_LZCNT ) str += "LZCNT ";
    if (features & CPU_FEATURE_BMI1  ) str += "BMI1 ";
    if (features & CPU_FEATURE_BMI2  ) str += "BMI2 ";
    if (features & CPU_FEATURE_KNC   ) str += "KNC ";
    if (features & CPU_FEATURE_AVX512F) str += "AVX512F ";
    if (features & CPU_FEATURE_AVX512DQ) str += "AVX512DQ ";
    if (features & CPU_FEATURE_AVX512PF) str += "AVX512PF ";
    if (features & CPU_FEATURE_AVX512ER) str += "AVX512ER ";
    if (features & CPU_FEATURE_AVX512CD) str += "AVX512CD ";
    if (features & CPU_FEATURE_AVX512BW) str += "AVX512BW ";
    if (features & CPU_FEATURE_AVX512VL) str += "AVX512VL ";
    if (features & CPU_FEATURE_AVX512IFMA) str += "AVX512IFMA ";
    if (features & CPU_FEATURE_AVX512VBMI) str += "AVX512VBMI ";
    return str;
  }

  std::string stringOfISA (int isa)
  {
    if (isa == SSE) return "SSE";
    if (isa == SSE2) return "SSE2";
    if (isa == SSE3) return "SSE3";
    if (isa == SSSE3) return "SSSE3";
    if (isa == SSE41) return "SSE4_1";
    if (isa == SSE42) return "SSE4_2";
    if (isa == AVX) return "AVX";
    if (isa == AVXI) return "AVXI";
    if (isa == AVX2) return "AVX2";
    if (isa == KNC) return "KNC";
    if (isa == AVX512KNL) return "AVX512KNL";
    if (isa == AVX512SKX) return "AVX512SKX";
    return "UNKNOWN";
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Windows Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ospcommon
{
  std::string getExecutableFileName() {
    char filename[1024];
    if (!GetModuleFileName(nullptr, filename, sizeof(filename))) return std::string();
    return std::string(filename);
  }

  size_t getNumberOfLogicalThreads()
  {
    static int nThreads = -1;
    if (nThreads != -1) return nThreads;

    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    typedef WORD (WINAPI *GetActiveProcessorGroupCountFunc)();
    typedef DWORD (WINAPI *GetActiveProcessorCountFunc)(WORD);
    HMODULE hlib = LoadLibrary("Kernel32");
    GetActiveProcessorGroupCountFunc pGetActiveProcessorGroupCount = (GetActiveProcessorGroupCountFunc)GetProcAddress(hlib, "GetActiveProcessorGroupCount");
    GetActiveProcessorCountFunc      pGetActiveProcessorCount      = (GetActiveProcessorCountFunc)     GetProcAddress(hlib, "GetActiveProcessorCount");

    if (pGetActiveProcessorGroupCount && pGetActiveProcessorCount &&
       ((osvi.dwMajorVersion > 6) || ((osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion >= 1))))
    {
      int groups = pGetActiveProcessorGroupCount();
      int totalProcessors = 0;
      for (int i = 0; i < groups; i++)
        totalProcessors += pGetActiveProcessorCount(i);
      nThreads = totalProcessors;
    }
    else
    {
      SYSTEM_INFO sysinfo;
      GetSystemInfo(&sysinfo);
      nThreads = sysinfo.dwNumberOfProcessors;
    }
    return nThreads;
  }

  int getTerminalWidth()
  {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) return 80;
    CONSOLE_SCREEN_BUFFER_INFO info = { { 0 } };
    GetConsoleScreenBufferInfo(handle, &info);
    return info.dwSize.X;
  }

  double getSeconds() {
    LARGE_INTEGER freq, val;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&val);
    return (double)val.QuadPart / (double)freq.QuadPart;
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// Linux Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __linux__

#include <stdio.h>
#include <unistd.h>

namespace ospcommon
{
  std::string getExecutableFileName()
  {
    char pid[32]; sprintf(pid, "/proc/%d/exe", getpid());
    char buf[1024];
    int bytes = readlink(pid, buf, sizeof(buf)-1);
    if (bytes != -1) buf[bytes] = '\0';
    return std::string(buf);
  }
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// Mac OS X Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __APPLE__

#include <mach-o/dyld.h>

namespace ospcommon
{
  std::string getExecutableFileName()
  {
    char buf[1024];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) return std::string();
    return std::string(buf);
  }
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// Unix Platform
////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN32

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>

namespace ospcommon
{
  size_t getNumberOfLogicalThreads() {
    static int nThreads = -1;
    if (nThreads == -1) nThreads = sysconf(_SC_NPROCESSORS_CONF);
    return nThreads;
  }

  int getTerminalWidth()
  {
    struct winsize info;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &info) < 0) return 80;
    return info.ws_col;
  }

  double getSeconds() {
    struct timeval tp; gettimeofday(&tp,nullptr);
    return double(tp.tv_sec) + double(tp.tv_usec)/1E6;
  }
}
#endif

