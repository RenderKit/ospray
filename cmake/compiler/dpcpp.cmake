## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(OSPRAY_CXX_FLAGS "-fno-strict-aliasing -Wno-narrowing -fhonor-infinities -fhonor-nans")

if(OSPRAY_STRICT_BUILD)
  # OK to turn off.
  set(OSPRAY_CXX_FLAGS "-Wno-c++98-compat-pedantic ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-documentation ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-documentation-unknown-command ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-zero-as-null-pointer-constant ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-newline-eof ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-keyword-macro ${OSPRAY_CXX_FLAGS}") #useful for unit testing
  set(OSPRAY_CXX_FLAGS "-Wno-undef ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-header-hygiene ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-covered-switch-default ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-date-time ${OSPRAY_CXX_FLAGS}")

  # Should try to fix and remove...
  set(OSPRAY_CXX_FLAGS "-Wno-unknown-warning-option ${OSPRAY_CXX_FLAGS}") #don't warn if pragmas are unknown
  set(OSPRAY_CXX_FLAGS "-Wno-conversion ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-reserved-id-macro ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-double-promotion ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-used-but-marked-unused ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-old-style-cast ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-missing-noreturn ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-missing-prototypes ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-shift-sign-overflow ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-padded ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-shadow-field-in-constructor ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-weak-template-vtables ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-weak-vtables ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-exit-time-destructors ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-global-constructors ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-unused-template ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-switch-enum ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-float-equal ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-cast-align ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-deprecated ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-mismatched-tags ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-disabled-macro-expansion ${OSPRAY_CXX_FLAGS}") #pesky 'stb_image.h'...
  set(OSPRAY_CXX_FLAGS "-Wno-over-aligned ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-shadow ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-format-nonliteral ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-cast-qual ${OSPRAY_CXX_FLAGS}") #Embree v3.x issue
  set(OSPRAY_CXX_FLAGS "-Wno-extra-semi-stmt ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-shadow-field ${OSPRAY_CXX_FLAGS}")
  set(OSPRAY_CXX_FLAGS "-Wno-alloca ${OSPRAY_CXX_FLAGS}")

  # Options selected for Clang 5.0+
  set(OSPRAY_CXX_FLAGS "-Weverything ${OSPRAY_CXX_FLAGS}")
endif()

if(OSPRAY_WARN_AS_ERRORS)
  set(OSPRAY_CXX_FLAGS "-Werror ${OSPRAY_CXX_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS "${OSPRAY_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

if (APPLE)
  set(CMAKE_CXX_FLAGS "-mmacosx-version-min=10.9 ${CMAKE_CXX_FLAGS}") # we only use MacOSX 10.9 features
endif()

# enable -static-intel
# Note: this doesn't seem to be used/relevant to the DPC++ compiler? Maybe just the
# DPCPP release one? The nightly clang compiler doesn't recognize this flag
#set(CMAKE_EXE_LINKER_FLAGS "-static-intel ${CMAKE_EXE_LINKER_FLAGS}")
#set(CMAKE_SHARED_LINKER_FLAGS "-static-intel ${CMAKE_SHARED_LINKER_FLAGS}")
#set(CMAKE_STATIC_LINKER_FLAGS "-static-intel ${CMAKE_STATIC_LINKER_FLAGS}")
#set(CMAKE_MODULE_LINKER_FLAGS "-static-intel ${CMAKE_MODULE_LINKER_FLAGS}")

# TODO: These flags aren't recognized by IntelLLVM (the nightly dpcpp compiler)
# but at least the nan one does show up in the released dpcpp compiler.
# Are they being removed? or added?
#set(CMAKE_CXX_FLAGS "-fhonor-nan-compares -fhonor-infinities ${CMAKE_CXX_FLAGS}")

# SYCL flags to match Embree
list(APPEND OSPRAY_CXX_FLAGS_SYCL
  -Wno-mismatched-tags
  -Wno-pessimizing-move
  -Wno-reorder
  -Wno-unneeded-internal-declaration
  -Wno-delete-non-abstract-non-virtual-dtor
  -Wno-dangling-field
  -Wno-unknown-pragmas
  -Wno-logical-op-parentheses
  -fsycl
  -fsycl-unnamed-lambda
  -fsycl-device-code-split=per_kernel
  -Xclang -fsycl-allow-func-ptr)

# FIXME: debug information generation takes forever in SYCL
# TODO: Still true?
list(APPEND OSPRAY_CXX_FLAGS_SYCL -g0)
# FIXME: assertion still not working in SYCL
# TODO: Still true?
#list(APPEND OSPRAY_CXX_FLAGS_SYCL -UDEBUG -DNDEBUG)

# IGC options from Embree
# Enable __noinline
list(APPEND OSPRAY_IGC_OPTIONS "EnableOCLNoInlineAttr=0")
# This works around some IGC bug in spill compression
# TODO: Still true?
list(APPEND OSPRAY_IGC_OPTIONS "VISAOptions=-scratchAllocForStackInKB 128 -nospillcompression")

# Allow printf inside indirectly callable function, right now I have this in all for testing
# TODO: Should only enable for debug builds, and this needs to be done using a generator expression
# if we want to support it in VS
list(APPEND OSPRAY_IGC_OPTIONS "ForceInlineStackCallWithImplArg=0" "EnableGlobalStateBuffer=1")

option(OSPRAY_IGC_ENABLE_ZE_BINARY "Enable ZEBinary (for GTPin)" OFF)
if (OSPRAY_IGC_ENABLE_ZE_BINARY)
  list(APPEND OSPRAY_IGC_OPTIONS "EnableZEBinary=1")
endif()

# This significantly improves compile times on 17028 and up, though also impacts performance some
option(OSPRAY_IGC_FAST_COMPILE
  "Pass flags to improve compilation speed at the cost of some optimization" OFF)
if (OSPRAY_IGC_FAST_COMPILE)
  set(OSPRAY_IGC_FAST_COMPILE_UNIT_SIZE_THRESHOLD "18000" CACHE STRING
    "Set the partition unit unit size threshold (default 18000)")

  list(APPEND OSPRAY_IGC_OPTIONS "PartitionUnit=1")
  list(APPEND OSPRAY_IGC_OPTIONS "UnitSizeThreshold=${OSPRAY_IGC_FAST_COMPILE_UNIT_SIZE_THRESHOLD}")
endif()
#list(APPEND OSPRAY_IGC_OPTIONS "ForceOCLSIMDWidth=8")

# Development option to dump shaders, when we compile AOT this has to be done at build time
option(OSPRAY_IGC_DUMP_SHADERS "Dump IGC shaders during build" OFF)
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
option(OSPRAY_DPCPP_LARGEGRF "Enable DPC++ Large GRF Support" OFF)
if (OSPRAY_DPCPP_LARGEGRF)
  list(APPEND OSPRAY_OCL_OPTIONS "-internal_options -cl-intel-256-GRF-per-thread")
endif()

string(REPLACE ";" "," OSPRAY_IGC_OPTIONS_STR "${OSPRAY_IGC_OPTIONS}")

set(OSPRAY_OCL_OPTIONS_STR "${OSPRAY_OCL_OPTIONS}")
string(REPLACE ";" " " OSPRAY_OCL_OPTIONS_STR "${OSPRAY_OCL_OPTIONS}")

set(OSPRAY_OCL_OTHER_OPTIONS_STR "${OSPRAY_OCL_OTHER_OPTIONS}")
string(REPLACE ";" " " OSPRAY_OCL_OTHER_OPTIONS_STR "${OSPRAY_OCL_OTHER_OPTIONS}")


