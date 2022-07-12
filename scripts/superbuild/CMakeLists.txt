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
  set(ISPC_VERSION "1.18.0" CACHE STRING "Which version of ISPC to download?")
  mark_as_advanced(CLEAR ISPC_VERSION)
  if (ISPC_VERSION STREQUAL "1.18.0")
    if (APPLE)
      set(ISPC_HASH "d1435b541182406ff6b18446d31ecceef0eae3aed7654391ae676d3142e0000d")
    elseif (WIN32)
      set(ISPC_HASH "9210bb2d9d3711367faccb37acf49966696132560b565471c1c6121f4924a17e")
    else()
      set(ISPC_HASH "6c379bb97962e9de7d24fd48b3f7e647dc42be898e9d187948220268c646b692")
    endif()
  endif()
else()
  mark_as_advanced(FORCE ISPC_VERSION)
endif()

set(RKCOMMON_VERSION "1.10.0" CACHE STRING "Which version of rkcommon to build?")
if (RKCOMMON_VERSION STREQUAL "1.10.0")
  set(RKCOMMON_HASH "eb968868b85254680c59ff3e091e9a7634d407cf0375d2b52840bf9044716a48")
endif()

option(DOWNLOAD_TBB "Download TBB or use the one found in the system environment?" ON)
if (DOWNLOAD_TBB)
  set(TBB_VERSION "2021.5.0" CACHE STRING "Which version of TBB to download?")
  mark_as_advanced(CLEAR TBB_VERSION)
  option(BUILD_TBB_FROM_SOURCE "Build TBB from source or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_TBB_FROM_SOURCE)
  if (TBB_VERSION STREQUAL "2021.5.0")
    if (BUILD_TBB_FROM_SOURCE)
      set(TBB_HASH "83ea786c964a384dd72534f9854b419716f412f9d43c0be88d41874763e7bb47")
    else()
      if (APPLE)
        set(TBB_HASH "388c1c25314e3251e38c87ade2323af74cdaae2aec9b68e4c206d61c30ef9c33")
      elseif (WIN32)
        set(TBB_HASH "096c004c7079af89fe990bb259d58983b0ee272afa3a7ef0733875bfe09fcd8e")
      else()
        set(TBB_HASH "74861b1586d6936b620cdab6775175de46ad8b0b36fa6438135ecfb8fb5bdf98")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE TBB_VERSION)
  mark_as_advanced(FORCE BUILD_TBB_FROM_SOURCE)
endif()

option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" ON)
set(EMBREE_VERSION "3.13.3" CACHE STRING "Which version of Embree to build?")
if (EMBREE_VERSION STREQUAL "3.13.3")
  if (BUILD_EMBREE_FROM_SOURCE)
    set(EMBREE_HASH "e98db1522d377a1d47c04ff7db29ccb4207412751c33cb756de32b20ae66e9b1")
  else()
    if (APPLE)
      set(EMBREE_HASH "79f2e56b4697f833be9b5088bea9c7ce22119adacaf09390e23a7140f6383fd9")
    elseif (WIN32)
      set(EMBREE_HASH "1decef1ab8d0f5b2b4ef59bf70b303da05d0fe8d741d0e15bacaccd144055821")
    else()
      set(EMBREE_HASH "0c5884a6d3dffa082c329327276b91cc686ae0374e91a4c2cbea8cf6f7ddfeb4")
    endif()
  endif()
endif()

set(OPENVKL_VERSION "1.3.0" CACHE STRING "Which version of OpenVKL to build?")
if (OPENVKL_VERSION STREQUAL "1.3.0")
  set(OPENVKL_HASH "4e456dfa566a57e5f00da91a24ecbf76223d4e0722d12d978a9c07e0af26785d")
endif()

option(BUILD_OIDN "Build OpenImageDenoise as a part of the superbuild." OFF)
if (BUILD_OIDN)
  set(OIDN_VERSION "1.4.3" CACHE STRING "Which version of OpenImageDenoise to build?")
  mark_as_advanced(CLEAR OIDN_VERSION)
  option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_OIDN_FROM_SOURCE)
  if (OIDN_VERSION STREQUAL "1.4.3")
    if (APPLE)
      set(OIDN_HASH "3dffa9a9a0f2cb9a0d89cf42ee7e3acb0db5552f0128d7868af71e9b80d72ecf")
    elseif (WIN32)
      set(OIDN_HASH "5c0f3d8a4211dfd064f7baeb4403e33cba36035ad8b6c8a8c0cabb7c705db4f4")
    else()
      set(OIDN_HASH "f6ec3b996c69386bbc69f61bc0e6fd17765530e6aa788cfc7a5d3e739548cd58")
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
option(BUILD_OSPRAY_MODULE_CPU "Enable building module_cpu" ON)
option(BUILD_OSPRAY_MODULE_MPI "Enable building module_mpi" OFF)
option(BUILD_OSPRAY_MODULE_MULTIDEVICE "Enable building module_multidevice" OFF)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
  set(ISA_X86 OFF)
  set(ISA_NEON ON)
else()
  set(ISA_X86 ON)
  set(ISA_NEON OFF)
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
include(dep_openvkl)

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

## Build main projects ##
set(DEFAULT_BUILD_COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} ${PARALLEL_JOBS_OPTS})

if (BUILD_OIDN OR BUILD_OSPRAY_MODULE_CPU OR BUILD_OSPRAY_MODULE_MPI)
  set(OSPRAY_ENABLE_MODULES ON)
endif()

if (BUILD_OSPRAY_MODULE_MPI)
  set(OSPRAY_MODULE_MPI ON)
  include(dep_snappy)
endif()

if (BUILD_OSPRAY_MODULE_MULTIDEVICE)
  set(OSPRAY_ENABLE_MODULES ON)
  set(OSPRAY_MODULE_MULTIDEVICE ON)
endif()

if (BUILD_OSPRAY_ANARI)
  include(anari-ospray.cmake)
endif()

if (NOT BUILD_DEPENDENCIES_ONLY)
  include(build_ospray.cmake)
endif()
