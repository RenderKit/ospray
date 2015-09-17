## ======================================================================== ##
## Copyright 2009-2015 Intel Corporation                                    ##
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
IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)

  IF(${CMAKE_MAJOR_VERSION} EQUAL 3)
    CMAKE_POLICY(SET CMP0042 NEW)
  ENDIF(${CMAKE_MAJOR_VERSION} EQUAL 3)
ENDIF(COMMAND cmake_policy)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
SET(OSPRAY_VERSION 0.8.2)
SET(OSPRAY_SOVERSION 0)

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

MARK_AS_ADVANCED(CMAKE_INSTALL_PREFIX)

SET(OSPRAY_BINARY_DIR ${PROJECT_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${OSPRAY_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${OSPRAY_BINARY_DIR})
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})

MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH)
MARK_AS_ADVANCED(EXECUTABLE_OUTPUT_PATH)
MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)

# add experimental KNL/AVX512 build option - not fully supported, yet, so keep it hidden
OPTION(OSPRAY_BUILD_ENABLE_KNL "Enable experimental 'Knights Landing' build?")
MARK_AS_ADVANCED(OSPRAY_BUILD_ENABLE_KNL)

# compiler selection for the non-MIC part
IF (WIN32)
  SET(OSPRAY_COMPILER "MSVC" CACHE INTERNAL "MSVC used on Windows.")
ELSE()
  IF (APPLE)
    SET(OSPRAY_COMPILER "CLANG" CACHE STRING "compiler to use (ICC,GCC, or CLANG)")
    SET_PROPERTY(CACHE OSPRAY_COMPILER PROPERTY STRINGS ICC GCC CLANG)
  ELSE(APPLE)
    SET(OSPRAY_COMPILER "GCC" CACHE STRING "compiler to use (ICC or GCC)")
    SET_PROPERTY(CACHE OSPRAY_COMPILER PROPERTY STRINGS ICC GCC)
  ENDIF(APPLE)
ENDIF()

IF (WIN32)
  SET(OSPRAY_BUILD_MIC_SUPPORT OFF CACHE INTERNAL "OSPRay with KNC not supported on Windows.")
  SET(OSPRAY_BUILD_MPI_DEVICE OFF CACHE INTERNAL "OSPRay with MPI Remote/Distributed not supported on Windows.")
ELSE()
  OPTION(OSPRAY_BUILD_MIC_SUPPORT "Build OSPRay with KNC Support?")
  OPTION(OSPRAY_BUILD_MPI_DEVICE "Add MPI Remote/Distributed rendering support?")
ENDIF()

# the arch we're targeting for the non-MIC/non-xeon phi part of ospray
SET(OSPRAY_BUILD_ISA "ALL" CACHE STRING "Target ISA (SSE, AVX, AVX2, or ALL)")
IF (OSPRAY_BUILD_ENABLE_KNL)
  SET_PROPERTY(CACHE OSPRAY_BUILD_ISA PROPERTY STRINGS ALL SSE AVX AVX2 AVX512)
  OPTION(OSPRAY_ISPC_KNL_NATIVE "Enable ISPC's native KNL build") # TODO: autodetect
ELSE()
  SET_PROPERTY(CACHE OSPRAY_BUILD_ISA PROPERTY STRINGS ALL SSE AVX AVX2)
ENDIF()

# make ospray target and compiler uppercase - we need this in some other parts of the build system
STRING(TOUPPER ${OSPRAY_COMPILER} OSPRAY_COMPILER)
STRING(TOUPPER ${OSPRAY_BUILD_ISA} OSPRAY_BUILD_ISA)

SET(OSPRAY_MIC ${OSPRAY_BUILD_MIC_SUPPORT})
SET(OSPRAY_MPI ${OSPRAY_BUILD_MPI_DEVICE})


##############################################################
# cmake modules
##############################################################
INCLUDE(cmake/ospray.cmake)
INCLUDE(cmake/ispc.cmake)
INCLUDE(cmake/configure_embree.cmake)
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
