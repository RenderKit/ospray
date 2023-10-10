## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## Global settings ##

cmake_minimum_required(VERSION 3.7)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX
      "${CMAKE_BINARY_DIR}/install"
      CACHE STRING "Final install location." FORCE)
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type." FORCE)
endif()
set(CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
if (WIN32)
  if (NOT OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET)
    set(CMAKE_CONFIGURATION_TYPES "${CONFIGURATION_TYPES}"
        CACHE STRING "List of generated configurations." FORCE)
    set(OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET ON
        CACHE INTERNAL "Default CMake configuration types set.")
  endif()
else()
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})
endif()
if (NOT DEPENDENCIES_BUILD_TYPE)
  set(DEPENDENCIES_BUILD_TYPE Release)
endif()

## Establish project ##

project(OSPRay_superbuild)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "OSPRay can only be built on 64-bit platforms. This
  problem most commonly happens on Windows, where the right solution is to
  switch to a 64-bit generator: retry running CMake with '-G \"Visual
  Studio 15 2017 Win64\"'")
else()
  message(STATUS "Proceeding with a 64-bit generator. (${CMAKE_SIZEOF_VOID_P})")
endif()

if (POLICY CMP0114)
  # allow old CMake behavior: NO_DEPENDS for ExternalProject_Add_StepTargets
  cmake_policy(SET CMP0114 OLD) # 3.19
endif ()

if (POLICY CMP0135)
  # use timestamp of extraction of downloaded archives
  cmake_policy(SET CMP0135 NEW) # 3.24
endif ()

include(ExternalProject)
include(GNUInstallDirs)
include(ProcessorCount)
include(CMakeDependentOption)

## Superbuild options ##

ProcessorCount(PROCESSOR_COUNT)

if(NOT PROCESSOR_COUNT EQUAL 0)
  set(BUILD_JOBS ${PROCESSOR_COUNT} CACHE STRING "Number of build jobs '-j <n>'")
else()
  set(BUILD_JOBS 4 CACHE STRING "Number of build jobs '-j <n>'")
endif()

option(ALWAYS_REBUILD "Force every project to always be rebuilt?" OFF)

option(DOWNLOAD_ISPC "Download ISPC or use the one found in the system environment?" ON)
if (DOWNLOAD_ISPC)
  set(ISPC_VERSION "1.21.1" CACHE STRING "Which version of ISPC to download?")
  mark_as_advanced(CLEAR ISPC_VERSION)
  option(BUILD_ISPCRT_FROM_SOURCE "Build ISPCRT or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_ISPCRT_FROM_SOURCE)
  if (ISPC_VERSION STREQUAL "1.21.1")
    set(ISPC_SOURCE_HASH "af3b05babc2260d0af11e48ca894c6255c0ec94635fdac955496513ced85f196")
    if (APPLE)
      set(ISPC_HASH "2b38919ea2fa5fa243ae5e8e64cb9f64b80d95381300b906e565d07a4ad49517")
    elseif (WIN32)
      set(ISPC_HASH "406ebdf2a062a58922e05e504885401e5e920e5560c681b815e6650e03076b06")
    else()
      set(ISPC_HASH "3e034f6c7faf5cbc3ed87e2879a15e1d56095c2093ee59e5c6e7f89551e39e95")
    endif()
  endif()
else()
  mark_as_advanced(FORCE ISPC_VERSION)
endif()

set(RKCOMMON_VERSION "1.12.0" CACHE STRING "Which version of rkcommon to build?")
if (RKCOMMON_VERSION STREQUAL "1.12.0")
  set(RKCOMMON_HASH "514be81fb9bc95bf8a36ee9dfe1289a0d09ad0b9d76263866c284066449c75ae")
endif()

option(DOWNLOAD_TBB "Download TBB or use the one found in the system environment?" ON)
if (DOWNLOAD_TBB)
  set(TBB_VERSION "2021.10.0" CACHE STRING "Which version of TBB to download?")
  mark_as_advanced(CLEAR TBB_VERSION)
  option(BUILD_TBB_FROM_SOURCE "Build TBB from source or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_TBB_FROM_SOURCE)
  if (TBB_VERSION STREQUAL "2021.10.0")
    if (BUILD_TBB_FROM_SOURCE)
      set(TBB_HASH "78fb7bb29b415f53de21a68c4fdf97de8ae035090d9ee9caa221e32c6e79567c")
    else()
      if (APPLE)
        set(TBB_HASH "20899b66b8a42d649283209276e70dd7e10ab10d90fd3c7372fc6e28dbc9a94b")
      elseif (WIN32)
        set(TBB_HASH "d517205d6cc2f80a56500a7f7e1428d47e7cfc55ea66675027024c7b90047ecb")
      else()
        set(TBB_HASH "d5be4164a1f2e67a8c7bc927cbe2b36690815adb48d36e50b9e3b8afa4c99310")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE TBB_VERSION)
  mark_as_advanced(FORCE BUILD_TBB_FROM_SOURCE)
endif()

option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" OFF)
set(EMBREE_VERSION "4.3.0" CACHE STRING "Which version of Embree to build?")
if (EMBREE_VERSION STREQUAL "4.3.0")
  if (BUILD_EMBREE_FROM_SOURCE)
    set(EMBREE_HASH "e248e69b7a6debceb66b9f9af8ff5f7b46c19e696473b7ccaa3e3b7a37d65287")
  else()
    if (APPLE)
      set(EMBREE_HASH "eed7a677bb5944f90238ad61dfdb4d602ed4568b2085af9ffd0b5d90cceee181")
    elseif (WIN32)
      set(EMBREE_HASH "4efe5269353168063ecb3ae24876c8ac868d5644c12bae75c86084c566b2acdb")
      set(EMBREE_SYCL_HASH "468163dc7ea4c6e6bc2d706caec17e9c0acb1c9d80ee0e8bcf4bec49105c8e9e")
    else()
      set(EMBREE_HASH "93507aea3e35dcf1bc40aafa20ca9e882ded562a8909eeacc02d0b3de17691d0")
      set(EMBREE_SYCL_HASH "ad316966430b0520643125329b266a81f2a46e2547dd586168c7b337643fbaa8")
    endif()
  endif()
endif()

option(BUILD_OPENVKL "Build Open VKL as a part of the superbuild." ON)
if (BUILD_OPENVKL)
  set(OPENVKL_VERSION "2.0.0" CACHE STRING "Which version of OpenVKL to build?")
  if (OPENVKL_VERSION STREQUAL "2.0.0")
    set(OPENVKL_HASH "df8c253616c8748a5e926671ec42b3ba0a4dca1111ba474a8335275dc000b381")
  endif()
else()
  mark_as_advanced(FORCE OPENVKL_VERSION)
endif()

option(BUILD_OIDN "Build OpenImageDenoise as a part of the superbuild." ON)
if (BUILD_OIDN)
  set(OIDN_VERSION "2.1.0" CACHE STRING "Which version of OpenImageDenoise to build?")
  mark_as_advanced(CLEAR OIDN_VERSION)
  option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_OIDN_FROM_SOURCE)
  if (OIDN_VERSION STREQUAL "2.1.0")
    if (BUILD_OIDN_FROM_SOURCE)
      set(OIDN_HASH "ce144ba582ff36563d9442ee07fa2a4d249bc85aa93e5b25fc527ff4ee755ed6")
    else()
      if (APPLE)
        set(OIDN_HASH "f202a442ac7d208a12463c11f29f3e0a0c367ef73535c16d255ab0c6e091e83c")
      elseif (WIN32)
        set(OIDN_HASH "5033574ddc1126d99f814a2eb8406cceb11ee60571dfe405b07a509750597f00")
      else()
        set(OIDN_HASH "53d0dc4498796f1d94e4347c3131ba1a84ecf46bced8a7ada31776525937703e")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE OIDN_VERSION)
  mark_as_advanced(FORCE BUILD_OIDN_FROM_SOURCE)
endif()

option(BUILD_GLFW "Build glfw for OSPRay examples or use one in existing environment?" ON)

option(BUILD_DEPENDENCIES_ONLY "Don't build OSPRay itself, only its dependencies" OFF)
option(BUILD_OSPRAY_CI_EXTRAS "Enable OSPRay's extra compiler checks." OFF)

option(BUILD_OSPRAY_APPS "Enable building OSPRay apps" ON)
option(BUILD_OSPRAY_MODULE_MPI "Enable building module_mpi" OFF)
option(BUILD_OSPRAY_MODULE_MULTIDEVICE "Enable building module_multidevice" OFF)
if (NOT APPLE)
  option(BUILD_GPU_SUPPORT "Enable beta GPU support and building module_gpu" OFF)
endif()

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
  set(ISA_X86 OFF)
  if (APPLE)
    set(ISA_NEON OFF)
    set(ISA_NEON2X ON)
  else()
    set(ISA_NEON ON)
    set(ISA_NEON2X OFF)
  endif()
else()
  set(ISA_X86 ON)
  set(ISA_NEON OFF)
  set(ISA_NEON2X OFF)
endif()
option(BUILD_ISA_SSE4 "Build for SSE4" ${ISA_X86})
mark_as_advanced(BUILD_ISA_SSE4)
option(BUILD_ISA_AVX "Build for AVX" ${ISA_X86})
mark_as_advanced(BUILD_ISA_AVX)
option(BUILD_ISA_AVX2 "Build for AVX2" ${ISA_X86})
mark_as_advanced(BUILD_ISA_AVX2)
option(BUILD_ISA_AVX512 "Build for AVX512" ${ISA_X86})
mark_as_advanced(BUILD_ISA_AVX512)
option(BUILD_ISA_NEON "Build for NEON" ${ISA_NEON})
mark_as_advanced(BUILD_ISA_NEON)
option(BUILD_ISA_NEON2X "Build for double pumped NEON" ${ISA_NEON2X})
mark_as_advanced(BUILD_ISA_NEON2X)

if (BUILD_ISA_NEON AND BUILD_ISA_NEON2X)
  message(FATAL_ERROR "Only one Neon ISA may be enabled; choose either NEON or NEON2X")
endif()


option(INSTALL_IN_SEPARATE_DIRECTORIES
  "Install libraries into their own directories under CMAKE_INSTALL_PREFIX"
  ON
)

option(INSTALL_DEPENDENCIES "Install OSPRay dependencies" ON)

## Setup top-level variables ##

set(installDir ${CMAKE_INSTALL_PREFIX})

get_filename_component(INSTALL_DIR_ABSOLUTE
  ${installDir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})

if (${CMAKE_VERSION} VERSION_GREATER 3.11.4)
  set(PARALLEL_JOBS_OPTS -j ${BUILD_JOBS})
endif()

## Build dependencies ##
set(DEFAULT_BUILD_COMMAND ${CMAKE_COMMAND} --build . --config ${DEPENDENCIES_BUILD_TYPE} ${PARALLEL_JOBS_OPTS})

if (DOWNLOAD_ISPC)
  include(dep_ispc)
endif()

if (DOWNLOAD_TBB)
  include(dep_tbb)
endif()

include(dep_rkcommon)
include(dep_embree)

if (BUILD_GLFW)
  include(dep_glfw)
endif()

if (BUILD_OIDN)
  include(dep_oidn)
endif()

include(dep_glm)

if (BUILD_OSPRAY_APPS)
  include(dep_gtest)
  include(dep_benchmark)
endif()

if (BUILD_OSPRAY_MODULE_MPI)
  set(OSPRAY_MODULE_MPI ON)
  include(dep_snappy)
endif()

##### FIXME WA for VKL and OSPRay to build with GNU-style options
if (WIN32 AND ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
  set(CMAKE_C_COMPILER clang)
  set(CMAKE_CXX_COMPILER clang++)
endif()

if (BUILD_OPENVKL)
  include(dep_openvkl)
endif()


## Build main projects ##
set(DEFAULT_BUILD_COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} ${PARALLEL_JOBS_OPTS})

if (NOT BUILD_DEPENDENCIES_ONLY)
  include(build_ospray.cmake)
endif()
