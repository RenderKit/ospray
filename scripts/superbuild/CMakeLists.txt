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

set(BUILD_OSPCOMMON_VERSION "devel" CACHE STRING "Which version of ospcommon to build?")

option(BUILD_TBB_FROM_SOURCE "Build TBB from source or instead use one in the existing environment?" ON)

set(BUILD_EMBREE_VERSION "v3.6.1" CACHE STRING "Which version of Embree to build?")
option(BUILD_EMBREE_FROM_SOURCE "Build Embree or use pre-built version?" ON)

set(BUILD_OPENVKL_VERSION "v0.7.0" CACHE STRING "Which version of OpenVKL to build?")

option(BUILD_OIDN "Build OpenImageDenoise as a part of the superbuild." OFF)
set(BUILD_OIDN_VERSION "v1.0.0" CACHE STRING "Which version of OpenImageDenoise to build?")
option(BUILD_OIDN_FROM_SOURCE "Build OpenImageDenoise or use pre-built version?" ON)

option(BUILD_GLFW "Build glfw for OSPRay examples or use one in existing environment?" ON)

option(BUILD_DEPENDENCIES_ONLY "Don't build OSPRay itself, only its dependencies" OFF)
option(BUILD_OSPRAY_CI_TESTS "Enable OSPRay's tests." OFF)
option(BUILD_OSPRAY_CI_EXTRAS "Enable OSPRay's extra compiler checks." OFF)

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

if (BUILD_TBB_FROM_SOURCE)
  include(dep_tbb)
endif()

include(dep_ospcommon)
include(dep_openvkl)
include(dep_embree)

if (BUILD_GLFW)
  include(dep_glfw)
endif()

if (BUILD_OIDN)
  include(dep_oidn)
endif()

## Build OSPRay ##

set(COMPONENT_NAME ospray)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

string(REPLACE ";" "|" CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")

if (NOT BUILD_DEPENDENCIES_ONLY)
  ExternalProject_Add(${COMPONENT_NAME}
    PREFIX ${COMPONENT_NAME}
    DOWNLOAD_COMMAND ""
    STAMP_DIR ${COMPONENT_NAME}/stamp
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..
    BINARY_DIR ${COMPONENT_NAME}/build
    LIST_SEPARATOR | # Use the alternate list separator
    CMAKE_ARGS
      -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
      -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
      -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
      -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
      -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
      -DOSPRAY_BUILD_ISA=ALL
      -DOSPRAY_ENABLE_TESTING=${BUILD_OSPRAY_CI_TESTS}
      -DOSPRAY_MODULE_BILINEAR_PATCH=${BUILD_OSPRAY_CI_TESTS}
      -DOSPRAY_MODULE_DENOISER=${BUILD_OIDN}
      -DOSPRAY_STRICT_BUILD=${BUILD_OSPRAY_CI_EXTRAS}
      -DOSPRAY_WARN_AS_ERRORS=${BUILD_OSPRAY_CI_EXTRAS}
      $<$<BOOL:${DOWNLOAD_ISPC}>:-DISPC_EXECUTABLE=${ISPC_PATH}>
      $<$<BOOL:${BUILD_TBB_FROM_SOURCE}>:-DOSPCOMMON_TBB_ROOT=${TBB_PATH}>
    BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
    BUILD_ALWAYS OFF
  )

  ExternalProject_Add_StepDependencies(${COMPONENT_NAME}
  configure
    ospcommon
    embree
    OpenVKL
    $<$<BOOL:${BUILD_GLFW}>:glfw>
    $<$<BOOL:${DOWNLOAD_ISPC}>:ispc>
    $<$<BOOL:${BUILD_OIDN}>:OpenImageDenoise>
  )
endif()
