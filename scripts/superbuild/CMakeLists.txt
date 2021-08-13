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
  set(CMAKE_BUILD_TYPE Release)
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

option(DOWNLOAD_ISPC "Download ISPC or use the one found in ${PATH}?" ON)
set(ISPC_VERSION "1.16.1" CACHE STRING "Which version of ISCP to download?")
if (ISPC_VERSION STREQUAL "1.16.1")
  if (APPLE)
    set(ISPC_HASH "7dbce602d97227a9603aabfae6dc3b3aa24d1cd44f0ccfb5ae47ecd4d68e988e")
  elseif (WIN32)
    set(ISPC_HASH "b34de2c36aff2afaa56b669ea41f9e614a045564ca74fc0b138e17ccea4880b7")
  else()
    set(ISPC_HASH "88db3d0461147c10ed81053a561ec87d3e14265227c03318f4fcaaadc831037f")
  endif()
endif()

set(RKCOMMON_VERSION "1.7.0" CACHE STRING "Which version of rkcommon to build?")
if (RKCOMMON_VERSION STREQUAL "1.7.0")
  set(RKCOMMON_HASH "b2eabd1dc56fd9de7cae320711ae07696385dc3151a56b63562396d0f5e2708e")
endif()

option(DOWNLOAD_TBB "Download TBB or use the only found in the system environment?" ON)
set(TBB_VERSION "2021.2.0" CACHE STRING "Which version of TBB to download?")
if (TBB_VERSION STREQUAL "2021.2.0")
  if (APPLE)
    set(TBB_HASH "ed6545190b747af9585ab8eedd5c8ccdf7cef304b6caa8db25803389fb24ec62")
  elseif (WIN32)
    set(TBB_HASH "9be37b1cb604a5905db0a15b2b893d85579fd0b2f1024859e1f75e96d7331a02")
  else()
    set(TBB_HASH "bcaa0aa7c0d2851a730cb0f64ad120f06a7b63811c8cc49e51ea8e1579f51b05")
  endif()
endif()

option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" ON)
set(EMBREE_VERSION "3.13.1" CACHE STRING "Which version of Embree to build?")
if (EMBREE_VERSION STREQUAL "3.13.1")
  if (BUILD_EMBREE_FROM_SOURCE)
    set(EMBREE_HASH "51c7fd6cbef79fd49b5a7fd2a1150922a369d183ab6ce5a6845cc73d2308b33d")
  else()
    if (APPLE)
      set(EMBREE_HASH "1bac7a2a63798a3d04e009d93de06f39875c48c86333f8435db8ea052db2e8ea")
    elseif (WIN32)
      set(EMBREE_HASH "5590ec5d4971d2c022cbb1d950f3a3b9666ec193c08b42eecf7decf5b5fd3833")
    else()
      set(EMBREE_HASH "7ad9ae04124acad4c4746a9d80d54b457cf078e381383bbb482c3e25e007c70b")
    endif()
  endif()
endif()

set(OPENVKL_VERSION "1.0.0" CACHE STRING "Which version of OpenVKL to build?")
if (OPENVKL_VERSION STREQUAL "1.0.0")
  set(OPENVKL_HASH "c2df70bec784a5ae42e2931d76d4feae51700c0b42cd5c2428b8ce6a64fffc34")
endif()

option(BUILD_OIDN "Build OpenImageDenoise as a part of the superbuild." OFF)
option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" OFF)
set(OIDN_VERSION "1.4.1" CACHE STRING "Which version of OpenImageDenoise to build?")
if (OIDN_VERSION STREQUAL "1.4.1")
  if (APPLE)
    set(OIDN_HASH "0ac6b2e16ff5f1b7ff75f19e5889f291bb36d66b84446bee06ff169cfe19f265")
  elseif (WIN32)
    set(OIDN_HASH "4ea0492de4fc47951ab5fc53b4e3433f39677d3fbfc1541eabc4ccee82bfc4d1")
  else()
    set(OIDN_HASH "efdf8fafb47017d7b1b321cbc88cb1bfcbdbe2746be57ed4a2ae90431fc9eb3a")
  endif()
endif()

option(BUILD_GLFW "Build glfw for OSPRay examples or use one in existing environment?" ON)

option(BUILD_DEPENDENCIES_ONLY "Don't build OSPRay itself, only its dependencies" OFF)
option(BUILD_OSPRAY_CI_EXTRAS "Enable OSPRay's extra compiler checks." OFF)

option(BUILD_OSPRAY_APPS "Enable building OSPRay apps" ON)
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

if (BUILD_OSPRAY_MODULE_MPI)
  set(OSPRAY_ENABLE_MODULES ON)
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
