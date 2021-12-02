## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## Global settings ##

cmake_minimum_required(VERSION 3.1)

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
  set(ISPC_VERSION "1.16.1" CACHE STRING "Which version of ISPC to download?")
  mark_as_advanced(CLEAR ISPC_VERSION)
  if (ISPC_VERSION STREQUAL "1.16.1")
    if (APPLE)
      set(ISPC_HASH "7dbce602d97227a9603aabfae6dc3b3aa24d1cd44f0ccfb5ae47ecd4d68e988e")
    elseif (WIN32)
      set(ISPC_HASH "b34de2c36aff2afaa56b669ea41f9e614a045564ca74fc0b138e17ccea4880b7")
    else()
      set(ISPC_HASH "88db3d0461147c10ed81053a561ec87d3e14265227c03318f4fcaaadc831037f")
    endif()
  endif()
else()
  mark_as_advanced(FORCE ISPC_VERSION)
endif()

set(RKCOMMON_VERSION "1.8.0" CACHE STRING "Which version of rkcommon to build?")
if (RKCOMMON_VERSION STREQUAL "1.8.0")
  set(RKCOMMON_HASH "d9593de25a9b705d7d56f53d655c71901a25be21d88aa493d39d2d6e61ed7393")
endif()

option(DOWNLOAD_TBB "Download TBB or use the one found in the system environment?" ON)
if (DOWNLOAD_TBB)
  set(TBB_VERSION "2021.4.0" CACHE STRING "Which version of TBB to download?")
  mark_as_advanced(CLEAR TBB_VERSION)
  option(BUILD_TBB_FROM_SOURCE "Build TBB from source or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_TBB_FROM_SOURCE)
  if (TBB_VERSION STREQUAL "2021.4.0")
    if (BUILD_TBB_FROM_SOURCE)
      set(TBB_HASH "08ade531be2e4e904eb6bec8e01da51eb3b0e4e86738128eb2722b95e3fcb5e6")
    else()
      if (APPLE)
        set(TBB_HASH "519d599939c41808d6b9170253b4c993b40d481f2c56b1a3cf390aea37ddb355")
      elseif (WIN32)
        set(TBB_HASH "3868c557739a7a5b74c985571648c066167fd7a0a8c63bdac00a6cfeeb58037f")
      else()
        set(TBB_HASH "f1d26b9f3741e5d573050eef2902fcd739d2913c990f4c879a310e543fc5ffd5")
      endif()
    endif()
  endif()
else()
  mark_as_advanced(FORCE TBB_VERSION)
  mark_as_advanced(FORCE BUILD_TBB_FROM_SOURCE)
endif()

option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" ON)
set(EMBREE_VERSION "3.13.2" CACHE STRING "Which version of Embree to build?")
if (EMBREE_VERSION STREQUAL "3.13.2")
  if (BUILD_EMBREE_FROM_SOURCE)
    set(EMBREE_HASH "eaa7a8ecd78594fb9eed75b2abbabd30dd68afb49556c250799daaeec016237c")
  else()
    if (APPLE)
      set(EMBREE_HASH "6e9442e516cd54c2e7f6454c90fb8cda5721d76a14d29880ffa387820a486762")
    elseif (WIN32)
      set(EMBREE_HASH "76570583a3d3e78f74b3cde2b0bbff8b0cc527959cc68b9e501b295e3aa7a960")
    else()
      set(EMBREE_HASH "8142c1fa0e8e89e279581e873f558b008a3d49b9b1e0091393e50377bcc52639")
    endif()
  endif()
endif()

set(OPENVKL_VERSION "1.1.0" CACHE STRING "Which version of OpenVKL to build?")
if (OPENVKL_VERSION STREQUAL "1.1.0")
  set(OPENVKL_HASH "2fd751f5eaa37342bd4574d3fdc0cf82e36b67646442a71c3ebe3aa859ccbc13")
endif()

option(BUILD_OIDN "Build OpenImageDenoise as a part of the superbuild." OFF)
if (BUILD_OIDN)
  set(OIDN_VERSION "1.4.2" CACHE STRING "Which version of OpenImageDenoise to build?")
  mark_as_advanced(CLEAR OIDN_VERSION)
  option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" OFF)
  mark_as_advanced(CLEAR BUILD_OIDN_FROM_SOURCE)
  if (OIDN_VERSION STREQUAL "1.4.2")
    if (APPLE)
      set(OIDN_HASH "176df94914d3bf920987f71650de05d6335f058fd631d2a5caa0e4e65f8e4fab")
    elseif (WIN32)
      set(OIDN_HASH "5d688d60b2abb3bc2aa19a357ef2347747635c08b3fc7fb95e0abee9a2202e70")
    else()
      set(OIDN_HASH "2d0eb873b79935166321df7197659e09ad5fc8a3aca3865793d12bb2b58f608f")
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
