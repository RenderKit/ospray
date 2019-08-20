## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

## Global settings ##

cmake_minimum_required(VERSION 3.1)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/dependencies
)

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

include(ExternalProject)
include(GNUInstallDirs)

## Superbuild options ##

set(BUILD_JOBS 4 CACHE STRING "Number of build jobs '-j <n>'")

set(BUILD_OSPCOMMON_VERSION "master" CACHE STRING "Which version of ospcommon to build?")

set(BUILD_EMBREE_VERSION "3.5.2" CACHE STRING "Which version of Embree to build?")
option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" ON)

option(BUILD_OIDN "Build OpenImageDenoise as a part of the superbuild." OFF)
set(BUILD_OIDN_VERSION "1.0.0" CACHE STRING "Which version of OpenImageDenoise to build?")
option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" ON)

option(BUILD_DEPENDENCIES_ONLY "Don't build OSPRay itself, only its dependencies" OFF)
option(BUILD_OSPRAY_CI_TESTS "Enable OSPRay's tests." OFF)
option(BUILD_OSPRAY_CI_EXTRAS "Enable OSPRay's extra compiler checks." OFF)
option(BUILD_OSPRAY_MPI "Enable OSPRay's MPI module (requires MPI pre-installed)" OFF)

option(INSTALL_IN_SEPARATE_DIRECTORIES
  "Install libraries into their own directories under CMAKE_INSTALL_PREFIX"
  ON
)

## Setup top-level variables ##

set(installDir ${CMAKE_INSTALL_PREFIX})

get_filename_component(INSTALL_DIR_ABSOLUTE
  ${installDir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})

if (${CMAKE_MINOR_VERSION} GREATER_EQUAL 12)
  set(PARALLEL_JOBS_OPTS -j ${NUM_BUILD_JOBS})
endif()

set(DEFAULT_BUILD_COMMAND cmake --build . --config release ${PARALLEL_JOBS_OPTS})

## Build dependencies ##

include(dep_ispc)
include(dep_tbb)
include(dep_ospcommon)
include(dep_embree)
if (BUILD_OIDN)
  include(dep_oidn)
endif()

## Build OSPRay ##

if (NOT BUILD_DEPENDENCIES_ONLY)
  ExternalProject_Add(ospray
    PREFIX ospray
    DOWNLOAD_COMMAND ""
    STAMP_DIR ospray/stamp
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..
    BINARY_DIR ospray/build
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_DIR_ABSOLUTE}/ospray
      -DOSPRAY_BUILD_ISA=ALL
      -DOSPRAY_ENABLE_TESTING=${BUILD_OSPRAY_CI_TESTS}
      -DOSPRAY_MODULE_BILINEAR_PATCH=${BUILD_OSPRAY_CI_TESTS}
      -DOSPRAY_MODULE_DENOISER=${BUILD_OIDN}
      -DOSPRAY_MODULE_MPI=${BUILD_OSPRAY_MPI}
      -DOSPRAY_STRICT_BUILD=${BUILD_OSPRAY_CI_EXTRAS}
      -DOSPRAY_WARN_AS_ERRORS=${BUILD_OSPRAY_CI_EXTRAS}
      -DOpenImageDenoise_DIR:PATH=${OIDN_PATH}
      -Dembree_DIR:PATH=${EMBREE_PATH}
      -Dospcommon_DIR:PATH=${OSPCOMMON_PATH}
      -DISPC_EXECUTABLE=${ISPC_PATH}
    BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
    BUILD_ALWAYS OFF
  )

  if (BUILD_OIDN)
    ExternalProject_Add_StepDependencies(ospray configure ospcommon embree OpenImageDenoise)
  else()
    ExternalProject_Add_StepDependencies(ospray configure ospcommon embree)
  endif()
endif()
