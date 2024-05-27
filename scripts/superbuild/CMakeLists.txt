## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## Global settings ##

cmake_minimum_required(VERSION 3.7)
set(CMAKE_INSTALL_MESSAGE LAZY)

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

option(INSTALL_IN_SEPARATE_DIRECTORIES
  "Install libraries into their own directories under CMAKE_INSTALL_PREFIX"
  ON
)

option(INSTALL_DEPENDENCIES "Install OSPRay dependencies" ON)

if (APPLE)
  set(BUILD_GPU_SUPPORT OFF)
else()
  option(BUILD_GPU_SUPPORT "Enable beta GPU support and building module_gpu" OFF)
endif()

option(DOWNLOAD_ISPC "Download ISPC or use the one found in the system environment?" ON)
if (DOWNLOAD_ISPC)
  set(ISPC_VERSION "1.24.0" CACHE STRING "Which version of ISPC to download?")
  mark_as_advanced(CLEAR ISPC_VERSION)
  if (ISPC_VERSION STREQUAL "1.24.0")
    if (APPLE)
      if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
        set(ISPC_HASH "7f3891d0157aed3cab159fbc5235235b62797053db9387f5a61c8d0a22369ae0")
      else()
        set(ISPC_HASH "986eb172fe9db3e8da560e9d0d788832991638fab61ca80587d87eb175ffb520")
      endif()
    elseif (WIN32)
      set(ISPC_HASH "a7c21cb2434f5364acbdf0933af6de49198458ed6f0b62012e03c3325c972649")
    else()
      if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
        set(ISPC_HASH "890ad5f31581091bf4ae9f235a7bb6fd9cabcbd6cf1670104ad61f1ad2da4c76")
      else()
        set(ISPC_HASH "79885d4482356dba7c01da312b6449fd575b097845ebcb90344dac5c178b43f5")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE ISPC_VERSION)
endif()

set(RKCOMMON_VERSION "devel" CACHE STRING "Which version of rkcommon to build?")
if (RKCOMMON_VERSION STREQUAL "1.13.0")
  set(RKCOMMON_HASH "9d360ce89de1842d4ae81b561ce150efd4fd812bd9e8b1d42d3934c528de637c")
endif()

option(DOWNLOAD_TBB "Download TBB or use the one found in the system environment?" ON)
if (DOWNLOAD_TBB)
  set(TBB_VERSION "2021.13.0" CACHE STRING "Which version of TBB to download?")
  mark_as_advanced(CLEAR TBB_VERSION)
  option(BUILD_TBB_FROM_SOURCE "Build TBB from source or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_TBB_FROM_SOURCE)
  if (TBB_VERSION STREQUAL "2021.13.0")
    if (BUILD_TBB_FROM_SOURCE)
      set(TBB_HASH "3ad5dd08954b39d113dc5b3f8a8dc6dc1fd5250032b7c491eb07aed5c94133e1")
    else()
      if (APPLE)
        set(TBB_HASH "e9aa143de91f65ea15b62ceeba5845bbc914c882d32bf9c9a405e6b8b5a70ade")
      elseif (WIN32)
        set(TBB_HASH "0a7b19b52ec9d8eaa10cbbe396e955130815489c544642acecda8ef8d2d7f64d")
      else()
        set(TBB_HASH "f5c9304710051f0193a07fb91b6d6ada5a3e0a6d623951ee176b1897816ecf4c")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE TBB_VERSION)
  mark_as_advanced(FORCE BUILD_TBB_FROM_SOURCE)
endif()

option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" OFF)
set(EMBREE_VERSION "4.3.2" CACHE STRING "Which version of Embree to build?")
if (EMBREE_VERSION STREQUAL "4.3.2")
  if (BUILD_EMBREE_FROM_SOURCE)
    set(EMBREE_HASH "cecd1004bc18406a91ca6c27005cbb50ac916a3a4134abadd7d797907703be4e")
  else()
    if (APPLE)
      if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
        set(EMBREE_HASH "c413402605220cb64dc4406b770629a8303483f8775ed0c574cd6b0b38c57196")
      else()
        set(EMBREE_HASH "304554a704d943299e608c6990091f1906c154d2f2d110379e54eee9a0e0cdef")
      endif()
    elseif (WIN32)
      set(EMBREE_HASH "1b7e1cc3cf01e1084e11e49d8c5dd9a7a3daddf599cf781a97d9a2240821a018")
      set(EMBREE_SYCL_HASH "a62075daed73bc8d726531eba4a55b39fb992ad599a794a592144ac181e5e0e4")
    else()
      set(EMBREE_HASH "eda5b439f09fdbb72f0f71b52afd8b9818ee9e15d75dde4d9a6ad8f29beaa8eb")
      set(EMBREE_SYCL_HASH "05ef87ab4d39e4bb067e5cec727ee46a26ff5ed5f091654f13b3a7450beefa3f")
    endif()
  endif()
endif()

option(BUILD_OPENVKL "Build Open VKL as a part of the superbuild." ON)
if (BUILD_OPENVKL)
  set(OPENVKL_VERSION "2.0.1" CACHE STRING "Which version of OpenVKL to build?")
  mark_as_advanced(CLEAR OPENVKL_VERSION)
  # XXX need to build VKL from source when installing in single directory,
  # otherwise its packaged, stale Embree will overwrite the already installed
  # new Embree
  cmake_dependent_option(
    BUILD_OPENVKL_FROM_SOURCE "Build OpenVKL or use pre-built version?"
    OFF
    "INSTALL_IN_SEPARATE_DIRECTORIES OR NOT INSTALL_DEPENDENCIES"
    ON
  )
  mark_as_advanced(CLEAR BUILD_OPENVKL_FROM_SOURCE)
  if (OPENVKL_VERSION STREQUAL "2.0.1")
    if (BUILD_OPENVKL_FROM_SOURCE)
      set(OPENVKL_HASH "6676f2171f9d61844ef2a417df79275226026bc4210a4afc61da2160ecaaf5ed")
    else()
      if (APPLE)
        set(OPENVKL_HASH "b04ab7a3376334ec6cf58014e41052d6d1d07cbd49c02f9a0f461849b79a0f45")
      elseif(WIN32)
        set(OPENVKL_HASH "634500d449e80da324e317d78abb5c6a634ae8c3d8b049261d49c49e1e8c7daa")
      else()
        set(OPENVKL_HASH "74939c835ec533c9de7af975e5cd0ee6db74a748388d89c915e3a08ebf274ad0")
      endif()
    endif()
  endif()
  else()
    mark_as_advanced(FORCE OPENVKL_VERSION)
    mark_as_advanced(FORCE BUILD_OPENVKL_FROM_SOURCE)
endif()

option(BUILD_OIDN "Build OpenImageDenoise as a part of the superbuild." ON)
if (BUILD_OIDN)
  set(OIDN_VERSION "2.3.0" CACHE STRING "Which version of OpenImageDenoise to build?")
  mark_as_advanced(CLEAR OIDN_VERSION)
  option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_OIDN_FROM_SOURCE)
  if (OIDN_VERSION STREQUAL "2.3.0")
    if (BUILD_OIDN_FROM_SOURCE)
      set(OIDN_HASH "cce3010962ec84e0ba1acd8c9055a3d8de402fedb1b463517cfeb920a276e427")
    else()
      if (APPLE)
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
          set(OIDN_HASH "9dc42edba6461082e940c8c8cf19e6528d3d40f7f45c695fab33671b39414627")
        else()
          set(OIDN_HASH "8b3285d2d8c431c061a2cf0c5bf90fd519b5ad5fb6db828b07dc6662daa9e07b")
        endif()
      elseif (WIN32)
        set(OIDN_HASH "272781c9398eec7aeef9cd1be5b5aad7bb3debfc574308ce930b8e55ee51effd")
      else()
        set(OIDN_HASH "02693c3eb3888eeb0c59c9bb1142b28b899ac8dd02bad4397beaffb5f8bef863")
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
  # remember CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS before switching to GNU-style
  # frontend, which won't look for MSVCRT anymore
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
  include(InstallRequiredSystemLibraries)
  file(CONFIGURE OUTPUT ospray_system_runtime.cmake
       CONTENT "set(OSPRAY_INSTALL_SYSTEM_RUNTIME_LIBS \"@CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS@\")"
       @ONLY NEWLINE_STYLE WIN32)
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
