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
SET(OSPRAY_VERSION_MAJOR 1)
SET(OSPRAY_VERSION_MINOR 0)
SET(OSPRAY_VERSION_PATCH 0)
SET(OSPRAY_VERSION_GITHASH 0)
IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  FIND_PACKAGE(Git)
  IF(GIT_FOUND)
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE "OSPRAY_VERSION_GITHASH"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  ENDIF()
ENDIF()

SET(OSPRAY_VERSION
  ${OSPRAY_VERSION_MAJOR}.${OSPRAY_VERSION_MINOR}.${OSPRAY_VERSION_PATCH}
)
SET(OSPRAY_SOVERSION 0)


SET(CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
IF (WIN32)
  IF (NOT OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET)
    SET(CMAKE_CONFIGURATION_TYPES "${CONFIGURATION_TYPES}"
        CACHE STRING "List of generated configurations." FORCE)
    SET(OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET ON
        CACHE INTERNAL "Default CMake configuration types set.")
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

##############################################################
# CMake modules and macro files
##############################################################

INCLUDE(cmake/ospray.cmake)
INCLUDE(cmake/ispc.cmake)
INCLUDE(cmake/mpi.cmake)

IF(NOT WIN32)
  INCLUDE(cmake/doxygen.cmake)
ENDIF()

OSPRAY_CONFIGURE_COMPILER()

##############################################################
# OSPRay specific build options and configuration selection
##############################################################

OPTION(OSPRAY_USE_EXTERNAL_EMBREE
       "Use a pre-built Embree instead of the internally built version")

OPTION(OSPRAY_BUILD_ENABLE_KNL "Enable experimental 'Knights Landing' build?")
MARK_AS_ADVANCED(OSPRAY_BUILD_ENABLE_KNL)

OPTION(OSPRAY_VOLUME_VOXELRANGE_IN_APP "Move 'voxelrange' computations to app?")
MARK_AS_ADVANCED(OSPRAY_VOLUME_VOXELRANGE_IN_APP)

IF (WIN32)
  SET(OSPRAY_BUILD_MIC_SUPPORT OFF CACHE INTERNAL
      "OSPRay with KNC not supported on Windows.")
ELSE()
  OPTION(OSPRAY_BUILD_MIC_SUPPORT "Build OSPRay with KNC Support?")
  IF (OSPRAY_BUILD_MIC_SUPPORT AND NOT OSPRAY_COMPILER_ICC)
    MESSAGE(FATAL_ERROR "MIC support requires the Intel Compiler.")
  ENDIF()
ENDIF()

OPTION(OSPRAY_BUILD_MPI_DEVICE "Add MPI Remote/Distributed rendering support?")

# the arch we're targeting for the non-MIC/non-xeon phi part of ospray
SET(OSPRAY_BUILD_ISA "ALL" CACHE STRING "Target ISA (SSE, AVX, AVX2, or ALL)")
IF (OSPRAY_BUILD_ENABLE_KNL)
  SET_PROPERTY(CACHE OSPRAY_BUILD_ISA PROPERTY STRINGS ALL SSE AVX AVX2 AVX512)
ELSE()
  SET_PROPERTY(CACHE OSPRAY_BUILD_ISA PROPERTY STRINGS ALL SSE AVX AVX2)
ENDIF()

# make ISA target case-insensitive, used in configure_ospray() macro
STRING(TOUPPER ${OSPRAY_BUILD_ISA} OSPRAY_BUILD_ISA)

SET(OSPRAY_MIC ${OSPRAY_BUILD_MIC_SUPPORT})
SET(OSPRAY_MPI ${OSPRAY_BUILD_MPI_DEVICE})

##############################################################
# create binary packages; before any INSTALL() invocation/definition
##############################################################

INCLUDE(package)

##############################################################
# the OSPRay 'common' library
##############################################################

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
SET(OSPRAY_TARGET "intel64")
ADD_SUBDIRECTORY(common builddir/ospray_common/intel64)
IF (OSPRAY_MIC)
  SET(OSPRAY_TARGET "mic")
  ADD_SUBDIRECTORY(common builddir/ospray_common/mic)
ENDIF()

##############################################################
# the OSPRay library
##############################################################

SET(OSPRAY_TARGET "intel64")
ADD_SUBDIRECTORY(ospray builddir/ospray/intel64)
IF (OSPRAY_MIC)
  SET(OSPRAY_TARGET "mic")
  ADD_SUBDIRECTORY(ospray builddir/ospray/mic)
ENDIF()

##############################################################
# OSPRay modules
##############################################################

SET(OSPRAY_TARGET "intel64")
ADD_SUBDIRECTORY(modules builddir/modules/intel64)
IF (OSPRAY_MIC)
  SET(OSPRAY_TARGET "mic")
  ADD_SUBDIRECTORY(modules builddir/modules/mic)
ENDIF()

##############################################################
# OSPRay sample apps; at the end: they may need modules
##############################################################

SET(OSPRAY_TARGET "intel64")
ADD_SUBDIRECTORY(apps builddir/apps/intel64)
IF (OSPRAY_MIC)
  SET(OSPRAY_TARGET "mic")
  ADD_SUBDIRECTORY(apps builddir/apps/mic)
ENDIF()

# create a configure file that both ospray and ispc can read the cmake config from
# needs to be at the end, after all cache variables have been set
CONFIGURE_FILE(ospray/common/OSPConfig.h.in OSPConfig.h)
INSTALL(FILES ${CMAKE_BINARY_DIR}/OSPConfig.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray
  COMPONENT devel
)

# has to be last
INCLUDE(CPack)
