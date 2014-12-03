# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

PROJECT(OSPRay)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)

  IF(${CMAKE_MAJOR_VERSION} EQUAL 3)
    CMAKE_POLICY(SET CMP0042 NEW)
  ENDIF(${CMAKE_MAJOR_VERSION} EQUAL 3)
ENDIF(COMMAND cmake_policy)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
SET(OSPRAY_VERSION 0.5.0)
SET(OSPRAY_SOVERSION 0)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type: Debug Release RelWithDebInfo MinSizeRel." FORCE)
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
SET(OSPRAY_INTERSECTION_FILTER OFF CACHE BOOL "Enable Intersection Filters?")
MARK_AS_ADVANCED(OSPRAY_INTERSECTION_FILTER)

# compiler selection for the non-MIC part
SET(OSPRAY_COMPILER "GCC" CACHE STRING "Set to ICC, GCC, or CLANG")
# whether to build in MIC/xeon phi support
SET(OSPRAY_BUILD_MIC_SUPPORT OFF CACHE BOOL "Build OSPRay with MIC Support?")
# whehter to build in MIC/xeon phi support
SET(OSPRAY_BUILD_MPI_DEVICE OFF CACHE BOOL "Add MPI Remote/Distributed rendering support?")
MARK_AS_ADVANCED(OSPRAY_MPI)
# the arch we're targeting for the non-MIC/non-xeon phi part of ospray
SET(OSPRAY_XEON_TARGET "AVX" CACHE STRING "Target ISA on host (SSE,AVX,AVX2)")

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
