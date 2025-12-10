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
  # XXX 1.31.0 breaks clipping on AVX-512 https://github.com/ispc/ispc/issues/3891
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

set(RKCOMMON_VERSION "1.15.4" CACHE STRING "Which version of rkcommon to build?")
if (RKCOMMON_VERSION STREQUAL "1.15.4")
  set(RKCOMMON_HASH "d584cc3620e56f12f07b84358b33a14de8aed29149693a1fe6eeed64ef74ae9b")
endif()

option(DOWNLOAD_TBB "Download TBB or use the one found in the system environment?" ON)
if (DOWNLOAD_TBB)
  set(TBB_VERSION "2023.1.0" CACHE STRING "Which version of TBB to download?")
  mark_as_advanced(CLEAR TBB_VERSION)
  option(BUILD_TBB_FROM_SOURCE "Build TBB from source or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_TBB_FROM_SOURCE)
  if (TBB_VERSION STREQUAL "2023.1.0")
    if (BUILD_TBB_FROM_SOURCE)
      set(TBB_HASH "191288b52e1e6b17198000b64d77d194bb65e791be46ebc606e9b091781e2070")
    else()
      if (APPLE)
        set(TBB_HASH "7093fb44a793989b4d52ac3881dfa312dccf81f4d829f554da31528741f5e47d")
      elseif (WIN32)
        set(TBB_HASH "cf6ee0c600fcb5c3a9b65e3e6e4781669d06f1bb1e37970d145fcde08eed8da9")
      else()
        set(TBB_HASH "349d0e8b08cae4a5ab2668d54ff4e90b0fa012a332de6fb156961ddc119cd617")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE TBB_VERSION)
  mark_as_advanced(FORCE BUILD_TBB_FROM_SOURCE)
endif()

option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" OFF)
set(EMBREE_VERSION "4.3.3" CACHE STRING "Which version of Embree to build?")
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
  set(OPENVKL_VERSION "devel" CACHE STRING "Which version of OpenVKL to build?")
  mark_as_advanced(CLEAR OPENVKL_VERSION)
  # XXX need to build VKL from source when installing in single directory,
  # otherwise its packaged, stale Embree will overwrite the already installed
  # new Embree
  cmake_dependent_option(
    BUILD_OPENVKL_FROM_SOURCE "Build OpenVKL or use pre-built version?"
    ON #XXX OFF
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
  set(OIDN_VERSION "2.5.1" CACHE STRING "Which version of OpenImageDenoise to build?")
  mark_as_advanced(CLEAR OIDN_VERSION)
  option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_OIDN_FROM_SOURCE)
  if (OIDN_VERSION STREQUAL "2.5.1")
    if (BUILD_OIDN_FROM_SOURCE)
      set(OIDN_HASH "e71fd043a70f1cc80e301d1b90df6c1f536098c4dd94baa612742f6db3369c36")
    else()
      if (APPLE)
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
          set(OIDN_HASH "98e0aca8e7ab69e9f4f0191582a500fd8b0d9085d68662f18ab6469e934a6efd")
        else()
          set(OIDN_HASH "a4341639005a33ce0d944bd7c771ce34de1465be7eb3189b7c3b5b0064429832")
        endif()
      elseif (WIN32)
        set(OIDN_HASH "f11f91bc072a5e3a564515724cb72ab8fcfbc445c84a84c197ff9b16cc01396f")
      else()
        set(OIDN_HASH "743c3e2aff8c220d5d70fe6cb970fb3d36f2702d2693c61d1d148e404cf37cd6")
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

include(dep_glm)

if (BUILD_OSPRAY_APPS)
  include(dep_gtest)
  include(dep_benchmark)
endif()

if (BUILD_OSPRAY_MODULE_MPI)
  set(OSPRAY_MODULE_MPI ON)
  include(dep_snappy)
endif()

##### FIXME WA for Embree, VKL, OIDN and OSPRay to build with GNU-style options
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

if (BUILD_OIDN)
  include(dep_oidn)
endif()


## Build main projects ##
set(DEFAULT_BUILD_COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} ${PARALLEL_JOBS_OPTS})

if (NOT BUILD_DEPENDENCIES_ONLY)
  include(build_ospray.cmake)
endif()
