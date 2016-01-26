## ======================================================================== ##
## Copyright 2009-2016 Intel Corporation                                    ##
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

PROJECT(OSPRay)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
CMAKE_POLICY(SET CMP0003 NEW) # since 2.6
IF(POLICY CMP0015)
  CMAKE_POLICY(SET CMP0015 NEW) # since 2.8.1
ENDIF()
IF(POLICY CMP0042)
  CMAKE_POLICY(SET CMP0042 NEW) # since 3.0
ENDIF()
IF(POLICY CMP0054)
  CMAKE_POLICY(SET CMP0054 OLD) # since 3.1
ENDIF()

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
SET(OSPRAY_VERSION_MAJOR 0)
SET(OSPRAY_VERSION_MINOR 9)
SET(OSPRAY_VERSION_PATCH 0)
SET(OSPRAY_VERSION
  ${OSPRAY_VERSION_MAJOR}.${OSPRAY_VERSION_MINOR}.${OSPRAY_VERSION_PATCH}
)
SET(OSPRAY_SOVERSION 0)

MACRO(PRINT var)
  MESSAGE("${var} = ${${var}}")
ENDMACRO()

SET(CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
IF (WIN32)
  IF (NOT DEFAULT_CMAKE_CONFIGURATION_TYPES_SET)
    SET(CMAKE_CONFIGURATION_TYPES "${CONFIGURATION_TYPES}" CACHE STRING "List of generated configurations." FORCE)
    SET(DEFAULT_CMAKE_CONFIGURATION_TYPES_SET ON CACHE INTERNAL "Default CMake configuration types set.")
  ENDIF()
ELSE()
  IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type." FORCE)
    SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})
  ENDIF()
ENDIF()

SET(OSPRAY_BINARY_DIR ${PROJECT_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${OSPRAY_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${OSPRAY_BINARY_DIR})

# RPath settings
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_BINARY_DIR}:${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# add experimental KNL/AVX512 build option - not fully supported, yet, so keep it hidden
OPTION(OSPRAY_BUILD_ENABLE_KNL "Enable experimental 'Knights Landing' build?")
MARK_AS_ADVANCED(OSPRAY_BUILD_ENABLE_KNL)

OPTION(OSPRAY_VOLUME_VOXELRANGE_IN_APP "Move 'voxelrange' computations to app?" OFF)
MARK_AS_ADVANCED(OSPRAY_VOLUME_VOXELRANGE_IN_APP)

IF (WIN32)
  SET(OSPRAY_BUILD_MIC_SUPPORT OFF CACHE INTERNAL "OSPRay with KNC not supported on Windows.")
ELSE()
  OPTION(OSPRAY_BUILD_MIC_SUPPORT "Build OSPRay with KNC Support?")
ENDIF()

OPTION(OSPRAY_BUILD_MPI_DEVICE "Add MPI Remote/Distributed rendering support?")

# the arch we're targeting for the non-MIC/non-xeon phi part of ospray
SET(OSPRAY_BUILD_ISA "ALL" CACHE STRING "Target ISA (SSE, AVX, AVX2, or ALL)")
IF (OSPRAY_BUILD_ENABLE_KNL)
  SET_PROPERTY(CACHE OSPRAY_BUILD_ISA PROPERTY STRINGS ALL SSE AVX AVX2 AVX512)
  OPTION(OSPRAY_ISPC_KNL_NATIVE "Enable ISPC's native KNL build") # TODO: autodetect
ELSE()
  SET_PROPERTY(CACHE OSPRAY_BUILD_ISA PROPERTY STRINGS ALL SSE AVX AVX2)
ENDIF()

# make ospray target and compiler uppercase - we need this in some other parts of the build system
STRING(TOUPPER ${OSPRAY_BUILD_ISA} OSPRAY_BUILD_ISA)

SET(OSPRAY_MIC ${OSPRAY_BUILD_MIC_SUPPORT})
SET(OSPRAY_MPI ${OSPRAY_BUILD_MPI_DEVICE})

# directoriers for client CMake find_package config

SET(HEADER_DIR include/ospray)
IF(WIN32)
  SET(CMAKE_DIR CMake)
ELSE(WIN32)
  SET(CMAKE_DIR share/ospray)
ENDIF(WIN32)

# Make OpenMP available to all apps/libraries/modules
FIND_PACKAGE(OpenMP QUIET)
IF (OPENMP_FOUND)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
ENDIF()

##############################################################
# cmake modules
##############################################################

INCLUDE(cmake/ospray.cmake)
INCLUDE(cmake/ispc.cmake)

SET(OSPRAY_EMBREE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/ospray/embree-v2.7.1)


INCLUDE(cmake/mpi.cmake)
IF(NOT WIN32)
  INCLUDE(cmake/doxygen.cmake)
ENDIF()

SET(OSPRAY_INSTALL_TARGET ospray-${OSPRAY_VERSION}-${CMAKE_SYSTEM_NAME})
STRING(TOLOWER "${OSPRAY_INSTALL_TARGET}" OSPRAY_INSTALL_TARGET_LC)

##############################################################
# the ospray library
##############################################################

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
SET (OSPRAY_TARGET "intel64")
ADD_SUBDIRECTORY(ospray builddir/ospray/intel64)
IF (OSPRAY_MIC)
  SET (OSPRAY_TARGET "mic")
  ADD_SUBDIRECTORY(ospray builddir/ospray/mic)
ENDIF()

##############################################################
# ospray modules
##############################################################

SET (OSPRAY_TARGET "intel64")
ADD_SUBDIRECTORY(modules builddir/modules/intel64)
IF (OSPRAY_MIC)
  SET (OSPRAY_TARGET "mic")
  ADD_SUBDIRECTORY(modules builddir/modules/mic)
ENDIF()

##############################################################
# ospray sample apps at the end, they may need modules
##############################################################

SET (OSPRAY_TARGET "intel64")
ADD_SUBDIRECTORY(apps builddir/apps/intel64)
IF (OSPRAY_MIC)
  SET (OSPRAY_TARGET "mic")
  ADD_SUBDIRECTORY(apps builddir/apps/mic)
ENDIF()

# create a configure file that both ospray and ispc can read the cmake config from
# needs to be at the end, after all cache variables have been set
CONFIGURE_FILE(ospray/common/OSPConfig.h.in OSPConfig.h)
INSTALL(FILES ${CMAKE_BINARY_DIR}/OSPConfig.h DESTINATION include/ospray)

