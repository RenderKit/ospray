## Copyright 2009-2020 Intel Corporation
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

include(ExternalProject)
include(GNUInstallDirs)
include(ProcessorCount)

## Superbuild options ##

ProcessorCount(PROCESSOR_COUNT)

if(NOT PROCESSOR_COUNT EQUAL 0)
  set(BUILD_JOBS ${PROCESSOR_COUNT} CACHE STRING "Number of build jobs '-j <n>'")
else()
  set(BUILD_JOBS 4 CACHE STRING "Number of build jobs '-j <n>'")
endif()

option(ALWAYS_REBUILD "Force every project to always be rebuilt?" OFF)

option(DOWNLOAD_ISPC "Download ispc or use the one found in ${PATH}?" ON)

set(BUILD_OSPCOMMON_VERSION "v1.2.0" CACHE STRING "Which version of ospcommon to build?")

option(DOWNLOAD_TBB "Download TBB or use the only found in the system environment?" ON)
set(TBB_VERSION "2020.0" CACHE STRING "Which version of TBB to download?")

set(BUILD_EMBREE_VERSION "v3.8.0" CACHE STRING "Which version of Embree to build?")
option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" ON)

set(BUILD_OPENVKL_VERSION "v0.8.0" CACHE STRING "Which version of OpenVKL to build?")

option(BUILD_OIDN "Build OpenImageDenoise as a part of the superbuild." OFF)
set(BUILD_OIDN_VERSION "v1.1.0" CACHE STRING "Which version of OpenImageDenoise to build?")
option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" OFF)

option(BUILD_GLFW "Build glfw for OSPRay examples or use one in existing environment?" ON)

option(BUILD_DEPENDENCIES_ONLY "Don't build OSPRay itself, only its dependencies" OFF)
option(BUILD_OSPRAY_CI_TESTS "Enable OSPRay's tests." OFF)
option(BUILD_OSPRAY_CI_EXTRAS "Enable OSPRay's extra compiler checks." OFF)

option(BUILD_OSPRAY_MODULE_MPI "Enable fetch/build of module_mpi" OFF)
set(BUILD_OSPRAY_MODULE_MPI_VERSION "v2.0.1" CACHE STRING "What version of module_mpi?")

option(INSTALL_IN_SEPARATE_DIRECTORIES
  "Install libraries into their own directories under CMAKE_INSTALL_PREFIX"
  ON
)

## Setup top-level variables ##

set(installDir ${CMAKE_INSTALL_PREFIX})

get_filename_component(INSTALL_DIR_ABSOLUTE
  ${installDir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})

if (${CMAKE_VERSION} VERSION_GREATER 3.11.4)
  set(PARALLEL_JOBS_OPTS -j ${BUILD_JOBS})
endif()

set(DEFAULT_BUILD_COMMAND cmake --build . --config release ${PARALLEL_JOBS_OPTS})

## Build dependencies ##

if (DOWNLOAD_ISPC)
  include(dep_ispc)
endif()

if (DOWNLOAD_TBB)
  include(dep_tbb)
endif()

include(dep_ospcommon)
include(dep_embree)
include(dep_openvkl)

if (BUILD_GLFW)
  include(dep_glfw)
endif()

if (BUILD_OIDN)
  include(dep_oidn)
endif()

if (NOT BUILD_DEPENDENCIES_ONLY)
  include(build_ospray.cmake)
  if (BUILD_OSPRAY_MODULE_MPI)
    include(build_module_mpi.cmake)
  endif()
endif()
