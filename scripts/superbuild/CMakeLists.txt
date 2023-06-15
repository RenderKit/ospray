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
  set(ISPC_VERSION "1.20.0" CACHE STRING "Which version of ISPC to download?")
  mark_as_advanced(CLEAR ISPC_VERSION)
  if (ISPC_VERSION STREQUAL "1.20.0")
    if (APPLE)
      set(ISPC_HASH "2667fa119761d793c63b6c7ee44e104dcccdaccca8212802b340edb8f8e3027d")
    elseif (WIN32)
      set(ISPC_HASH "e212ebfb4e8afb57adc103a2579c52673a3ca49610fbc2a5eae643d3d378548d")
    else()
      set(ISPC_HASH "e6412b88aa312fcd10c46f92df0149ccc4d99e53552c4ce127aa6c634fe9b308")
    endif()
  endif()
else()
  mark_as_advanced(FORCE ISPC_VERSION)
endif()

set(RKCOMMON_VERSION "1.11.0" CACHE STRING "Which version of rkcommon to build?")
if (RKCOMMON_VERSION STREQUAL "1.11.0")
  set(RKCOMMON_HASH "8e70462a2d6a882040b9968ed7d820dbae56d02668041b186afccc9043ab3df9")
endif()

option(DOWNLOAD_TBB "Download TBB or use the one found in the system environment?" ON)
if (DOWNLOAD_TBB)
  set(TBB_VERSION "2021.9.0" CACHE STRING "Which version of TBB to download?")
  mark_as_advanced(CLEAR TBB_VERSION)
  option(BUILD_TBB_FROM_SOURCE "Build TBB from source or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_TBB_FROM_SOURCE)
  if (TBB_VERSION STREQUAL "2021.9.0")
    if (BUILD_TBB_FROM_SOURCE)
      set(TBB_HASH "fcebb93cb9f7e882f62cd351b1c093dbefdcae04b616227dc716b0a5efa9e8ab")
    else()
      if (APPLE)
        set(TBB_HASH "2892f2a51aa404ba95bf20b6a9d5763bb8b6250aa0dc40a876d72c94f35748f0")
      elseif (WIN32)
        set(TBB_HASH "0c4d96edd0469cc93f611f94365ec07f3a0dd529b96d4e6ceac378de63920b1c")
      else()
        set(TBB_HASH "1e8f4d584c209b1a1d1935e72c0a86c16e65e6d2859cb96736ec6ed72c6123f5")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE TBB_VERSION)
  mark_as_advanced(FORCE BUILD_TBB_FROM_SOURCE)
endif()

option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" OFF)
set(EMBREE_VERSION "4.1.0" CACHE STRING "Which version of Embree to build?")
if (EMBREE_VERSION STREQUAL "4.1.0")
  if (BUILD_EMBREE_FROM_SOURCE)
    set(EMBREE_HASH "0d98995712b8d10ba6dd861b037e5cd74285f6277e12b9b68c81b5aa093722b8")
  else()
    if (APPLE)
      set(EMBREE_HASH "2d32b650e0e88b067d2d856473dc70411b2f5b5d27ae95b70feafb1e39364078")
    elseif (WIN32)
      set(EMBREE_HASH "8972ad00497b06f6ed83750edf7ff57760b82e5c7262b4c40ce77599f516d197")
    else()
      set(EMBREE_HASH "8e5dd14c91054708fc589dd679e0fd7de37ebcf8e208e8bc254abc91f4c66c0b")
    endif()
  endif()
endif()

set(OPENVKL_VERSION "1.3.2" CACHE STRING "Which version of OpenVKL to build?")
if (OPENVKL_VERSION STREQUAL "1.3.2")
  set(OPENVKL_HASH "02d9c48d747a46aa921099b27cef06ad382dcd4d0e1657f74c5d1f289caa05e7")
endif()

option(BUILD_OIDN "Build OpenImageDenoise as a part of the superbuild." OFF)
if (BUILD_OIDN)
  set(OIDN_VERSION "2.0.0" CACHE STRING "Which version of OpenImageDenoise to build?")
  mark_as_advanced(CLEAR OIDN_VERSION)
  option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_OIDN_FROM_SOURCE)
  if (OIDN_VERSION STREQUAL "2.0.0")
    if (APPLE)
      set(OIDN_HASH "44055036c5ee3cfc26057d4ad3b6e7f0cbd1feb4230386a30eca115004308c9a")
    elseif (WIN32)
      set(OIDN_HASH "df0c6bf8185f9736dd10eb07e4bf1c68c6ef28c0cddd42a9555f9e8435a24e40")
    else()
      set(OIDN_HASH "8de608405ab4e687a154560245a4520cf44a5a917f7fa2ec2b53e885638e2865")
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
if (NOT APPLE)
  option(BUILD_GPU_SUPPORT "Enable GPU support EXPERIMENTAL" OFF)
  mark_as_advanced(BUILD_GPU_SUPPORT)
  if (BUILD_GPU_SUPPORT)
    set(BUILD_GPU_AOT_DEVICES "none" CACHE STRING "Devices to use for AOT compilation")
    set_property(CACHE BUILD_GPU_AOT_DEVICES PROPERTY STRINGS none dg2 pvc)
  endif()
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

if (NOT BUILD_DEPENDENCIES_ONLY)
  include(build_ospray.cmake)
endif()
