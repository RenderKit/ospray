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

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)

  IF(${CMAKE_MAJOR_VERSION} EQUAL 3)
    CMAKE_POLICY(SET CMP0042 NEW)
  ENDIF(${CMAKE_MAJOR_VERSION} EQUAL 3)
ENDIF(COMMAND cmake_policy)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
SET(OSPRAY_VERSION 0.7.2)
SET(OSPRAY_SOVERSION 0)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type." FORCE)
  SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
ENDIF(NOT CMAKE_BUILD_TYPE)

MARK_AS_ADVANCED(CMAKE_INSTALL_PREFIX)

SET(OSPRAY_BINARY_DIR ${PROJECT_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${OSPRAY_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${OSPRAY_BINARY_DIR})
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})

MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH)
MARK_AS_ADVANCED(EXECUTABLE_OUTPUT_PATH)
MARK_AS_ADVANCED(CMAKE_INSTALL_PREFIX)
MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)

# whether to enable intersection filters
OPTION(OSPRAY_INTERSECTION_FILTER "Enable Intersection Filters?")
MARK_AS_ADVANCED(OSPRAY_INTERSECTION_FILTER)

# compiler selection for the non-MIC part
IF (APPLE)
  SET(OSPRAY_DEFAULT_COMPILER "CLANG")
ELSE(APPLE)
  SET(OSPRAY_DEFAULT_COMPILER "GCC")
ENDIF(APPLE)
SET(OSPRAY_COMPILER ${OSPRAY_DEFAULT_COMPILER} CACHE STRING "Select which compiler to use.")
SET_PROPERTY(CACHE OSPRAY_COMPILER PROPERTY STRINGS ICC GCC CLANG)
# whether to build in MIC/xeon phi support
OPTION(OSPRAY_BUILD_MIC_SUPPORT "Build OSPRay with MIC Support?")
# whether to build in MPI support
OPTION(OSPRAY_BUILD_MPI_DEVICE "Add MPI Remote/Distributed rendering support?")
# the arch we're targeting for the non-MIC/non-xeon phi part of ospray
SET(OSPRAY_XEON_TARGET "AVX2" CACHE STRING "Maximum target ISA (e.g., 'avx2' means SSE,AVX,and AVX2).")
SET_PROPERTY(CACHE OSPRAY_XEON_TARGET PROPERTY STRINGS SSE AVX AVX2)

# make ospray target and compiler uppercase - we need this in some other parts of the build system
STRING(TOUPPER ${OSPRAY_XEON_TARGET} OSPRAY_XEON_TARGET)
STRING(TOUPPER ${OSPRAY_COMPILER} OSPRAY_COMPILER)


SET(OSPRAY_MIC ${OSPRAY_BUILD_MIC_SUPPORT})
SET(OSPRAY_MPI ${OSPRAY_BUILD_MPI_DEVICE})


##############################################################
# cmake modules
##############################################################
INCLUDE(cmake/ospray.cmake)
INCLUDE(cmake/ispc.cmake)
INCLUDE(cmake/configure_embree.cmake)
INCLUDE(cmake/mpi.cmake)
INCLUDE(cmake/doxygen.cmake)

SET(OSPRAY_INSTALL_TARGET ospray-${OSPRAY_VERSION}-${CMAKE_SYSTEM_NAME}-${OSPRAY_XEON_TARGET})
STRING(TOLOWER "${OSPRAY_INSTALL_TARGET}" OSPRAY_INSTALL_TARGET_LC)
SET(CMAKE_INSTALL_PREFIX 
  /tmp/$ENV{USER}/OSPRAY-RELEASES/${OSPRAY_INSTALL_TARGET_LC})
#  ${CMAKE_BINARY_DIR}/release/${OSPRAY_INSTALL_TARGET_LC})

##############################################################
# the ospray library
##############################################################

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
SET (OSPRAY_TARGET ${OSPRAY_XEON_TARGET})
ADD_SUBDIRECTORY(ospray builddir/ospray/intel64)
IF (OSPRAY_MIC)
  SET (OSPRAY_TARGET "MIC")
  ADD_SUBDIRECTORY(ospray builddir/ospray/mic)
ENDIF()

##############################################################
# ospray sample apps
##############################################################
SET (OSPRAY_TARGET ${OSPRAY_XEON_TARGET})
ADD_SUBDIRECTORY(apps builddir/apps/intel64)
IF (OSPRAY_MIC)
  SET (OSPRAY_TARGET "MIC")
  ADD_SUBDIRECTORY(apps builddir/apps/mic)
ENDIF()


##############################################################
# modules at the end, they may need utilities
##############################################################
SET (OSPRAY_TARGET ${OSPRAY_XEON_TARGET})
ADD_SUBDIRECTORY(modules builddir/modules/intel64)
IF (OSPRAY_MIC)
  SET (OSPRAY_TARGET "MIC")
  ADD_SUBDIRECTORY(modules builddir/modules/mic)
ENDIF()
