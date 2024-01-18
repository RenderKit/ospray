## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# FIXME CMake does not always set "-x c++" to compile .ispc files as C++ (SYCL)
# code, despite LANGUAGE is set for them
set(OSPRAY_COMPILER_NEEDS_X_CPP TRUE)

get_filename_component(SYCL_COMPILER_NAME ${CMAKE_CXX_COMPILER} NAME_WE)
if (WIN32 AND (SYCL_COMPILER_NAME STREQUAL "icx" OR SYCL_COMPILER_NAME STREQUAL "icpx"))
  include(msvc) # icx on Windows behaves like msvc
  set(CMAKE_CXX_FLAGS "/fp:precise ${CMAKE_CXX_FLAGS}")
  set(OSPRAY_COMPILER_NEEDS_X_CPP FALSE) # icx on Win does not support "-x"
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /Qoption,link,/DEPENDENTLOADFLAG:0x2000")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /Qoption,link,/DEPENDENTLOADFLAG:0x2000")
else()
  include(clang) # DPCPP compiler is based on Clang, so bring in clang options
  set(CMAKE_CXX_FLAGS "-ffp-model=precise ${CMAKE_CXX_FLAGS}")
endif()

# enable -static-intel
# Note: this doesn't seem to be used/relevant to the DPC++ compiler? Maybe just the
# DPCPP release one? The nightly clang compiler doesn't recognize this flag
#set(CMAKE_EXE_LINKER_FLAGS "-static-intel ${CMAKE_EXE_LINKER_FLAGS}")
#set(CMAKE_SHARED_LINKER_FLAGS "-static-intel ${CMAKE_SHARED_LINKER_FLAGS}")
#set(CMAKE_MODULE_LINKER_FLAGS "-static-intel ${CMAKE_MODULE_LINKER_FLAGS}")

# SYCL flags to match Embree
list(APPEND OSPRAY_CXX_FLAGS_SYCL
  -fsycl
  -fsycl-unnamed-lambda
  -Xclang -fsycl-allow-func-ptr

  -Wno-mismatched-tags
  -Wno-pessimizing-move
  -Wno-reorder
  -Wno-unneeded-internal-declaration
  -Wno-delete-non-abstract-non-virtual-dtor
  -Wno-dangling-field
  -Wno-unknown-pragmas
  -Wno-logical-op-parentheses
)

# IGC options from Embree
# This works around some IGC bug in spill compression
# TODO: Still true?
list(APPEND OSPRAY_IGC_OPTIONS "VISAOptions=-scratchAllocForStackInKB 128")

# Allow printf inside indirectly callable function, right now I have this in all for testing
# TODO: Should only enable for debug builds, and this needs to be done using a generator expression
# if we want to support it in VS
#list(APPEND OSPRAY_IGC_OPTIONS "ForceInlineStackCallWithImplArg=0" "EnableGlobalStateBuffer=1")

option(OSPRAY_IGC_ENABLE_ZE_BINARY "Enable ZEBinary (for GTPin)" OFF)
mark_as_advanced(OSPRAY_IGC_ENABLE_ZE_BINARY)
if (OSPRAY_IGC_ENABLE_ZE_BINARY)
  list(APPEND OSPRAY_IGC_OPTIONS "EnableZEBinary=1")
endif()

# This significantly improves compile times on 17028 and up, though also impacts performance some
option(OSPRAY_IGC_FAST_COMPILE
  "Pass flags to improve compilation speed at the cost of some optimization" OFF)
mark_as_advanced(OSPRAY_IGC_FAST_COMPILE)
if (OSPRAY_IGC_FAST_COMPILE)
  set(OSPRAY_IGC_FAST_COMPILE_UNIT_SIZE_THRESHOLD "18000" CACHE STRING
    "Set the partition unit unit size threshold (default 18000)")

  list(APPEND OSPRAY_IGC_OPTIONS "PartitionUnit=1")
  list(APPEND OSPRAY_IGC_OPTIONS "UnitSizeThreshold=${OSPRAY_IGC_FAST_COMPILE_UNIT_SIZE_THRESHOLD}")
endif()
#list(APPEND OSPRAY_IGC_OPTIONS "ForceOCLSIMDWidth=8")

# Development option to dump shaders, when we compile AOT this has to be done at build time
option(OSPRAY_IGC_DUMP_SHADERS "Dump IGC shaders during build" OFF)
mark_as_advanced(OSPRAY_IGC_DUMP_SHADERS)
if (OSPRAY_IGC_DUMP_SHADERS)
  list(APPEND OSPRAY_IGC_OPTIONS
    "ShaderDumpEnable=1"
    "ShowFullVectorsInShaderDumps=1"
    "DumpToCustomDir=${PROJECT_BINARY_DIR}/ospray_igc_shader_dump")
endif()

# enables support for buffers larger than 4GB
list(APPEND OSPRAY_OCL_OPTIONS -cl-intel-greater-than-4GB-buffer-required)
list(APPEND OSPRAY_OCL_OTHER_OPTIONS
  -cl-intel-force-global-mem-allocation
  -cl-intel-no-local-to-generic)

if (CMAKE_BUILD_TYPE MATCHES "Release")
  list(APPEND OSPRAY_OCL_OTHER_OPTIONS -O2)
elseif (CMAKE_BUILD_TYPE MATCHES "Debug")
  # SYCL applies some optimization flags by default, make sure we're
  # really building with no optimizations for debug builds. RelWithDebInfo
  # builds currently hit a compiler crash.
  list(APPEND OSPRAY_OCL_OTHER_OPTIONS -O0)
endif()

# Large GRF mode
option(OSPRAY_SYCL_LARGEGRF "Enable SYCL Large GRF Support" ON)
mark_as_advanced(OSPRAY_SYCL_LARGEGRF)
if (OSPRAY_SYCL_LARGEGRF)
  list(APPEND OSPRAY_OCL_OTHER_OPTIONS "-ze-opt-large-register-file")
endif()

string(REPLACE ";" "," OSPRAY_IGC_OPTIONS_STR "${OSPRAY_IGC_OPTIONS}")

set(OSPRAY_OCL_OPTIONS_STR "${OSPRAY_OCL_OPTIONS}")
string(REPLACE ";" " " OSPRAY_OCL_OPTIONS_STR "${OSPRAY_OCL_OPTIONS}")

set(OSPRAY_OCL_OTHER_OPTIONS_STR "${OSPRAY_OCL_OTHER_OPTIONS}")
string(REPLACE ";" " " OSPRAY_OCL_OTHER_OPTIONS_STR "${OSPRAY_OCL_OTHER_OPTIONS}")

set(OSPRAY_LINKER_FLAGS_SYCL -fsycl -Xsycl-target-backend=spir64 "${OSPRAY_OCL_OPTIONS_STR} -options \"${OSPRAY_OCL_OTHER_OPTIONS_STR} -igc_opts='${OSPRAY_IGC_OPTIONS_STR}'\"")
