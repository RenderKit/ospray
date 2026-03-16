## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## Global settings ##

cmake_minimum_required(VERSION 3.10)
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
  set(ISPC_VERSION "1.30.0" CACHE STRING "Which version of ISPC to download?")
  mark_as_advanced(CLEAR ISPC_VERSION)
  if (ISPC_VERSION STREQUAL "1.30.0")
    if (APPLE)
      if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
        set(ISPC_HASH "01b05547ee3bed3a428c1ba2e72ec98743c8ad848c7db11c725bcd1e77889626")
      else()
        set(ISPC_HASH "e415277f5c3fa238f040135085630436bc1a1b87c14cc6f507b7c47c1c25cfd9")
      endif()
    elseif (WIN32)
      set(ISPC_HASH "e126a78fd15f12475bac6204f2536b3986e07973127e4f8e7336c3d304c4a69f")
    else()
      if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
        set(ISPC_HASH "509399c399ec162d746889458a10cc13797a1aed1c0164b2bd3faddf7d023f13")
      else()
        set(ISPC_HASH "63e7d61037849fa1ed644f0398d21740ee9f880b9bf81f017c65eebe1d42c02b")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE ISPC_VERSION)
endif()

set(RKCOMMON_VERSION "1.15.2" CACHE STRING "Which version of rkcommon to build?")
if (RKCOMMON_VERSION STREQUAL "1.15.2")
  set(RKCOMMON_HASH "9128642e5187b963385dbf1655f3b143d3ef46310cc2e27114d220b347d0e2f2")
endif()

option(DOWNLOAD_TBB "Download TBB or use the one found in the system environment?" ON)
if (DOWNLOAD_TBB)
  set(TBB_VERSION "2022.3.0" CACHE STRING "Which version of TBB to download?")
  mark_as_advanced(CLEAR TBB_VERSION)
  option(BUILD_TBB_FROM_SOURCE "Build TBB from source or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_TBB_FROM_SOURCE)
  if (TBB_VERSION STREQUAL "2022.0.0")
    if (BUILD_TBB_FROM_SOURCE)
      set(TBB_HASH "01598a46c1162c27253a0de0236f520fd8ee8166e9ebb84a4243574f88e6e50a")
    else()
      if (APPLE)
        set(TBB_HASH "f9f9369e956bd1cb1efbe6eab400291970e55fad1f3ae2a33bc96d991882ed38")
      elseif (WIN32)
        set(TBB_HASH "e1b2373f25558bf47d16b4c89cf0a31e6689aaf7221400d209e8527afc7c9eee")
      else()
        set(TBB_HASH "7be34b376c9bdba9afe1ebacaf94ed6dc15af8377d73b539ec932f141195f6ec")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE TBB_VERSION)
  mark_as_advanced(FORCE BUILD_TBB_FROM_SOURCE)
endif()

option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" OFF)
set(EMBREE_VERSION "4.4.0" CACHE STRING "Which version of Embree to build?")
if (EMBREE_VERSION STREQUAL "4.3.3")
  if (BUILD_EMBREE_FROM_SOURCE)
    set(EMBREE_HASH "8a3bc3c3e21aa209d9861a28f8ba93b2f82ed0dc93341dddac09f1f03c36ef2d")
  else()
    if (APPLE)
      if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
        set(EMBREE_HASH "ee310bcd8480ed82f2be29289b9a811c83db6a36160c8afdcb61bb4de4d337df")
      else()
        set(EMBREE_HASH "98997523dfff8efd8120075f7feba00cfc765c992d758d9be12d56bfe6bec233")
      endif()
    elseif (WIN32)
      set(EMBREE_HASH "d4c07f88df9f009dd84e4e9b9dcec32ad7d96f927bd88de00b721b0923d481a9")
      set(EMBREE_SYCL_HASH "1675fd6907e4697400ce58942343fdf99fc4e6f0fa0b259d61755ae72aa9aa18")
    else()
      set(EMBREE_HASH "757e6e8b987d13ac34aa7c4c3657120fd54a78c2a1034e30dda5cd5df06f3cdd")
      set(EMBREE_SYCL_HASH "9952f9f8fe103449e18084c8bd822a7e3130844c54abfd164f1ce36e46b92572")
    endif()
  endif()
endif()

option(BUILD_OPENVKL "Build Open VKL as a part of the superbuild." ON)
if (BUILD_OPENVKL)
  set(OPENVKL_VERSION "2.0.2" CACHE STRING "Which version of OpenVKL to build?")
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
  if (OPENVKL_VERSION STREQUAL "2.0.2")
    if (BUILD_OPENVKL_FROM_SOURCE)
      set(OPENVKL_HASH "9e1c730c196dde51e80f58b5ceb8fa2ea92d42cc460d18f9660b23feb81ae06e")
    else()
      if (APPLE)
        set(OPENVKL_HASH "5657292515990b81eb0a7d8d45e6c30736d4df7ca2b4c2d7c9c50f809fb4b0eb")
      elseif(WIN32)
        set(OPENVKL_HASH "7ec05767fe939aa6a72145e65b5ae65b39d07d05c9abf95fdb560ce1399bc7c5")
      else()
        set(OPENVKL_HASH "5622f802ac41d9f5d4a478b4cb1c65ac7a678bde03a4732faeb26ede37908fa0")
      endif()
    endif()
  endif()
  else()
    mark_as_advanced(FORCE OPENVKL_VERSION)
    mark_as_advanced(FORCE BUILD_OPENVKL_FROM_SOURCE)
endif()

option(BUILD_OIDN "Build OpenImageDenoise as a part of the superbuild." ON)
if (BUILD_OIDN)
  set(OIDN_VERSION "2.4.1" CACHE STRING "Which version of OpenImageDenoise to build?")
  mark_as_advanced(CLEAR OIDN_VERSION)
  option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_OIDN_FROM_SOURCE)
  if (OIDN_VERSION STREQUAL "2.4.1")
    if (BUILD_OIDN_FROM_SOURCE)
      set(OIDN_HASH "9c7c77ae0d57e004479cddb7aaafd405c2cc745153bed4805413c21be610e17b")
    else()
      if (APPLE)
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
          set(OIDN_HASH "f1d7370bc09242bbd72d405b424ba240fd4d64103f3e607cdfeeaa2f2718cfb8")
        else()
          set(OIDN_HASH "b9addf2855ee36d7768fd02d4d540e64612096487bad302e608d04e639ae1584")
        endif()
      elseif (WIN32)
        set(OIDN_HASH "682d94ba57525ed177d73412e0ed903f576867bd048f830a5c6f63c56b25e8b8")
      else()
        set(OIDN_HASH "b69ca2443a226ef692ca46bdc4f89995b1e99091f2665b906cbe07e9673e48cc")
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

##### FIXME WA for Embree, VKL and OSPRay to build with GNU-style options
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

include(dep_embree)

if (BUILD_OPENVKL)
  include(dep_openvkl)
endif()


## Build main projects ##
set(DEFAULT_BUILD_COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} ${PARALLEL_JOBS_OPTS})

if (NOT BUILD_DEPENDENCIES_ONLY)
  include(build_ospray.cmake)
endif()
