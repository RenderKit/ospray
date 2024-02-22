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
  set(ISPC_VERSION "1.23.0" CACHE STRING "Which version of ISPC to download?")
  mark_as_advanced(CLEAR ISPC_VERSION)
  option(BUILD_ISPCRT_FROM_SOURCE "Build ISPCRT or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_ISPCRT_FROM_SOURCE)
  if (ISPC_VERSION STREQUAL "1.23.0")
    set(ISPC_SOURCE_HASH "d665f0d4160a8777cf478e5f806478bb817daa40b3111f366e07dc6615b0ebe7")
    if (APPLE)
      if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
        set(ISPC_HASH "2cf144aaa6d8117e3a9e0782984fa4cb45127387dd4fb385b187115d6c1a0d68")
      else()
        set(ISPC_HASH "b9e6dcc045f5a2e29a6c43354b6a747c4486a341608d785f5f99eca8ac207a72")
      endif()
    elseif (WIN32)
      set(ISPC_HASH "709350902381968ee58fd67e9aed63df99b1313bc55a94195977bcc8d90bdced")
    else()
      if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
        set(ISPC_HASH "f82a0464b8d172c2c015f4252154651c6a25061f7a47ffb7f7f5b785eddfc9cc")
      else()
        set(ISPC_HASH "9183b2dd128fa5ca30ab24ca2cf6730a11572e4efd82fd284d167109f9c9d077")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE ISPC_VERSION)
endif()

set(RKCOMMON_VERSION "1.13.0" CACHE STRING "Which version of rkcommon to build?")
if (RKCOMMON_VERSION STREQUAL "1.13.0")
  set(RKCOMMON_HASH "9d360ce89de1842d4ae81b561ce150efd4fd812bd9e8b1d42d3934c528de637c")
endif()

option(DOWNLOAD_TBB "Download TBB or use the one found in the system environment?" ON)
if (DOWNLOAD_TBB)
  set(TBB_VERSION "2021.11.0" CACHE STRING "Which version of TBB to download?")
  mark_as_advanced(CLEAR TBB_VERSION)
  option(BUILD_TBB_FROM_SOURCE "Build TBB from source or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_TBB_FROM_SOURCE)
  if (TBB_VERSION STREQUAL "2021.11.0")
    if (BUILD_TBB_FROM_SOURCE)
      set(TBB_HASH "2f0bfce641d238e80798fc48397d43821bd977d49c4e03bc785be363b7ab4742")
    else()
      if (APPLE)
        set(TBB_HASH "360bcb20bcdcd01e8492c32bba6d5d5baf4bc83f77fb9dbf1ff701ac816e3b44")
      elseif (WIN32)
        set(TBB_HASH "02f0e93600fba69bb1c00e5dd3f66ae58f56e5410342f6155455a95ba373b1b6")
      else()
        set(TBB_HASH "95659f4d7b1711c41ffa190561d4e5b6841efc8091549661c7a2e6207e0fa79b")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE TBB_VERSION)
  mark_as_advanced(FORCE BUILD_TBB_FROM_SOURCE)
endif()

option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" OFF)
set(EMBREE_VERSION "4.3.1" CACHE STRING "Which version of Embree to build?")
if (EMBREE_VERSION STREQUAL "4.3.1")
  if (BUILD_EMBREE_FROM_SOURCE)
    set(EMBREE_HASH "bdab87b285efa1a9f1f57fe74b2743c659c487fee7e32221db43a6b8f5e36e5f")
  else()
    if (APPLE)
      if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
        set(EMBREE_HASH "adfb26b8ae37618cea09dc3a63ac710dd96b665991218ed79d2733a6c0aa65b3")
      else()
        set(EMBREE_HASH "ffb446c5a688d961f4219f42446147632f5704374c5b2b148a1e2a20b7c99975")
      endif()
    elseif (WIN32)
      set(EMBREE_HASH "6e2b968d45c0895cda98fb44c42e214e8d382bcd08b0a548fe086b0864563c6a")
      set(EMBREE_SYCL_HASH "c03735ee4801b11bf8b191f521527a57e9d77d6a2b7dd4439fef801748814984")
    else()
      set(EMBREE_HASH "375e829dc31ac5c6c579de34e0b1527e088a3b296fb9d3f90ed892d29592f845")
      set(EMBREE_SYCL_HASH "f4e5d36be8b3cd2d3f57fab885ffb769e071f5ce7d2a9ddfba61dae69c18693b")
    endif()
  endif()
endif()

option(BUILD_OPENVKL "Build Open VKL as a part of the superbuild." ON)
if (BUILD_OPENVKL)
  set(OPENVKL_VERSION "2.0.1" CACHE STRING "Which version of OpenVKL to build?")
  if (OPENVKL_VERSION STREQUAL "2.0.1")
    set(OPENVKL_HASH "6676f2171f9d61844ef2a417df79275226026bc4210a4afc61da2160ecaaf5ed")
  endif()
else()
  mark_as_advanced(FORCE OPENVKL_VERSION)
endif()

option(BUILD_OIDN "Build OpenImageDenoise as a part of the superbuild." ON)
if (BUILD_OIDN)
  set(OIDN_VERSION "2.2.0" CACHE STRING "Which version of OpenImageDenoise to build?")
  mark_as_advanced(CLEAR OIDN_VERSION)
  option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_OIDN_FROM_SOURCE)
  if (OIDN_VERSION STREQUAL "2.2.0")
    if (BUILD_OIDN_FROM_SOURCE)
      set(OIDN_HASH "5864386628d35d2b555380969ac957dc52c714d01866f557d9d1400ee01e61f1")
    else()
      if (APPLE)
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
          set(OIDN_HASH "16cdaddabeb0f2af22fc8e466317c1a14a54b6ec03db2ac343e17fad40c70f3f")
        else()
          set(OIDN_HASH "eafef2feb1f622f1fd1985673407598ba8746257cbd6f3ba55c470e6e92ebf6c")
        endif()
      elseif (WIN32)
        set(OIDN_HASH "f570b95c023f71d453f11bd0b53066e7f147b6b191512e044fe4a68df2806afc")
      else()
        set(OIDN_HASH "d9a67ed99840a19333213981850db09e843443cce658f3643ed17990518ce8d9")
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
