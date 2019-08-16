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

cmake_minimum_required(VERSION 3.1)

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX
      "${CMAKE_BINARY_DIR}/install"
      CACHE STRING "Final install location." FORCE)
endif()

project(SuperbuildTest)

include(ExternalProject)
include(GNUInstallDirs)
set(installDir ${CMAKE_INSTALL_PREFIX})

# Options
set(NUM_BUILD_JOBS 4 CACHE STRING "Number of build jobs '-j <n>'")
set(BUILD_OSPCOMMON_VERSION "master" CACHE STRING "Which version of ospcommon to build?")
set(BUILD_EMBREE_VERSION "3.5.2" CACHE STRING "Which version of Embree to build?")
set(BUILD_OIDN_VERSION "1.0.0" CACHE STRING "Which version of OpenImageDenoise to build?")

option(BUILD_DEPENDENCIES_ONLY "Don't build OSPRay itself, only deps" OFF)

if (${CMAKE_MINOR_VERSION} GREATER_EQUAL 12)
  set(PARALLEL_JOBS_OPTS -j ${NUM_BUILD_JOBS})
endif()

set(DEFAULT_BUILD_COMMAND cmake --build . --config release ${PARALLEL_JOBS_OPTS})

## ISPC ##

if (APPLE)
  set(ISPC_URL http://sdvis.org/ospray/download/dependencies/osx/ispc-v1.12.0-macOS.tar.gz)
elseif(WIN32)
  set(ISPC_URL http://sdvis.org/ospray/download/dependencies/win/ispc-v1.12.0-windows.zip)
else()
  set(ISPC_URL http://sdvis.org/ospray/download/dependencies/linux/ispc-v1.12.0-linux.tar.gz)
endif()

get_filename_component(INSTALL_DIR_ABSOLUTE
  ${installDir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})

ExternalProject_Add(ispc
  PREFIX ispc
  INSTALL_DIR ${INSTALL_DIR_ABSOLUTE}/
  URL ${ISPC_URL}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    <SOURCE_DIR>/bin/ispc${CMAKE_EXECUTABLE_SUFFIX}
    <INSTALL_DIR>/ispc/bin/ispc${CMAKE_EXECUTABLE_SUFFIX}
  BUILD_ALWAYS OFF
)

set(ISPC_PATH "${INSTALL_DIR_ABSOLUTE}/ispc/bin/ispc${CMAKE_EXECUTABLE_SUFFIX}")

## TBB ##

ExternalProject_Add(tbb
  PREFIX tbb
  DOWNLOAD_DIR tbb
  STAMP_DIR tbb/stamp
  SOURCE_DIR tbb/src
  BINARY_DIR tbb/build
  INSTALL_DIR ${INSTALL_DIR_ABSOLUTE}/tbb
  URL "https://github.com/wjakob/tbb/archive/master.zip"
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_BUILD_TYPE=Release
    -DTBB_BUILD_TESTS=OFF
    -DTBB_BUILD_SHARED=ON
    -DTBB_BUILD_STATIC=OFF
  BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
  BUILD_ALWAYS OFF
)

set(TBB_INSTALL_LOCATION "${INSTALL_DIR_ABSOLUTE}/tbb")

## ospcommon ##

ExternalProject_Add(ospcommon
  PREFIX ospcommon
  DOWNLOAD_DIR ospcommon
  STAMP_DIR ospcommon/stamp
  SOURCE_DIR ospcommon/src
  BINARY_DIR ospcommon/build
  INSTALL_DIR ${INSTALL_DIR_ABSOLUTE}/ospcommon
  GIT_REPOSITORY https://github.com/ospray/ospcommon
  GIT_TAG master
  GIT_SHALLOW ON
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_BUILD_TYPE=Release
    -DOSPCOMMON_TBB_ROOT=${TBB_INSTALL_LOCATION}
  BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
  BUILD_ALWAYS OFF
)

set(OSPCOMMON_PATH "${INSTALL_DIR_ABSOLUTE}/ospcommon")
if (NOT WIN32)
  set(OSPCOMMON_PATH "${OSPCOMMON_PATH}/lib/cmake/ospcommon")
endif()

ExternalProject_Add_StepDependencies(ospcommon configure tbb)

## Embree ##

ExternalProject_Add(embree
  PREFIX embree
  DOWNLOAD_DIR embree
  STAMP_DIR embree/stamp
  SOURCE_DIR embree/src
  BINARY_DIR embree/build
  INSTALL_DIR ${INSTALL_DIR_ABSOLUTE}/embree
  URL "https://github.com/embree/embree/archive/v${BUILD_EMBREE_VERSION}.zip"
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DEMBREE_TUTORIALS=OFF
    -DEMBREE_TBB_ROOT=${TBB_INSTALL_LOCATION}
    -DEMBREE_ISPC_EXECUTABLE=${ISPC_PATH}
    -DCMAKE_BUILD_TYPE=Release
    -DBUILD_TESTING=OFF
  BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
  BUILD_ALWAYS OFF
)

set(EMBREE_PATH "${INSTALL_DIR_ABSOLUTE}/embree")
if (NOT WIN32)
  set(EMBREE_PATH "${EMBREE_PATH}/${CMAKE_INSTALL_LIBDIR}/cmake/embree-${BUILD_EMBREE_VERSION}")
endif()

ExternalProject_Add_StepDependencies(embree configure ispc tbb)

## OpenImageDenoise ##

ExternalProject_Add(oidn
  PREFIX oidn
  DOWNLOAD_DIR oidn
  STAMP_DIR oidn/stamp
  SOURCE_DIR oidn/src
  BINARY_DIR oidn/build
  INSTALL_DIR ${INSTALL_DIR_ABSOLUTE}/OpenImageDenoise
  GIT_REPOSITORY https://github.com/OpenImageDenoise/oidn
  GIT_TAG v${BUILD_OIDN_VERSION}
  GIT_SHALLOW ON
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_BUILD_TYPE=Release
    -DTBB_ROOT=${TBB_ROOT}
  BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
  BUILD_ALWAYS OFF
)

set(OIDN_PATH "${INSTALL_DIR_ABSOLUTE}/OpenImageDenoise")
if (NOT WIN32)
  set(OIDN_PATH "${OIDN_PATH}/${CMAKE_INSTALL_LIBDIR}/cmake/OpenImageDenoise")
endif()

ExternalProject_Add_StepDependencies(oidn configure tbb ispc)

## OSPRay ##

if (NOT BUILD_DEPENDENCIES_ONLY)
  set(ENV{TBB_ROOT} ${TBB_ROOT})

  ExternalProject_Add(ospray
    PREFIX ospray
    DOWNLOAD_COMMAND ""
    STAMP_DIR ospray/stamp
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..
    BINARY_DIR ospray/build
    INSTALL_DIR ${INSTALL_DIR_ABSOLUTE}/ospray
    CMAKE_ARGS
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      -DOSPRAY_MODULE_DENOISER=ON
      -DOpenImageDenoise_DIR:PATH=${OIDN_PATH}
      -Dembree_DIR:PATH=${EMBREE_PATH}
      -Dospcommon_DIR:PATH=${OSPCOMMON_PATH}
      -DISPC_EXECUTABLE=${ISPC_PATH}
      -DCMAKE_BUILD_TYPE=Release
    BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
    BUILD_ALWAYS OFF
  )

  ExternalProject_Add_StepDependencies(ospray configure ospcommon embree oidn)
endif()
