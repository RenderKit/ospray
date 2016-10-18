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

##############################################################
# Global configuration options
##############################################################

SET(OSPRAY_VERSION_MAJOR 1)
SET(OSPRAY_VERSION_MINOR 1)
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

IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  SET(OSPRAY_DEBUG_BUILD          ON )
  SET(OSPRAY_RELWITHDEBINFO_BUILD OFF)
  SET(OSPRAY_RELEASE_BUILD        OFF)
ELSEIF("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
  SET(OSPRAY_DEBUG_BUILD          OFF)
  SET(OSPRAY_RELWITHDEBINFO_BUILD ON )
  SET(OSPRAY_RELEASE_BUILD        OFF)
ELSE()# Release
  SET(OSPRAY_DEBUG_BUILD          OFF)
  SET(OSPRAY_RELWITHDEBINFO_BUILD OFF)
  SET(OSPRAY_RELEASE_BUILD        ON )
ENDIF()

SET(OSPRAY_BINARY_DIR ${PROJECT_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${OSPRAY_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${OSPRAY_BINARY_DIR})

#include bindir - that's where ispc puts generated header files
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
SET(OSPRAY_BINARY_DIR ${CMAKE_BINARY_DIR})
SET(OSPRAY_DIR ${PROJECT_SOURCE_DIR})

IF (WIN32)
  # avoid problematic min/max defines of windows.h
  ADD_DEFINITIONS(-DNOMINMAX)
ENDIF()

##############################################################
# create binary packages; before any INSTALL() invocation/definition
##############################################################

OPTION(OSPRAY_ZIP_MODE "Use tarball/zip CPack generator instead of RPM" ON)
MARK_AS_ADVANCED(OSPRAY_ZIP_MODE)

INCLUDE(package)

##############################################################
# OSPRay specific build options and configuration selection
##############################################################

OPTION(OSPRAY_USE_EXTERNAL_EMBREE
       "Use a pre-built Embree instead of the internally built version" ON)

OPTION(OSPRAY_USE_EMBREE_STREAMS "Enable Streams if using Embree v2.10 or later")

OPTION(OSPRAY_USE_HIGH_QUALITY_BVH
       "Takes slighly longer to build but offers higher ray tracing performance; recommended when using Embree v2.11 or later")

OPTION(OSPRAY_VOLUME_VOXELRANGE_IN_APP "Move 'voxelrange' computations to app?")
MARK_AS_ADVANCED(OSPRAY_VOLUME_VOXELRANGE_IN_APP)

# Configure MIC support
IF (WIN32)
  SET(OSPRAY_BUILD_MIC_SUPPORT OFF CACHE INTERNAL
      "OSPRay with KNC not supported on Windows.")
ELSE()
  OPTION(OSPRAY_BUILD_MIC_SUPPORT "Build OSPRay with KNC Support?")
  IF (OSPRAY_BUILD_MIC_SUPPORT AND NOT OSPRAY_COMPILER_ICC)
    MESSAGE(FATAL_ERROR "MIC support requires the Intel Compiler.")
  ELSEIF (OSPRAY_BUILD_MIC_SUPPORT)
    # Build COI device?
    OPTION(OSPRAY_BUILD_COI_DEVICE
           "Build COI Device for OSPRay's MIC support?" ON)
  ENDIF()
ENDIF()

OPTION(OSPRAY_BUILD_MPI_DEVICE "Add MPI Remote/Distributed rendering support?")

SET(OSPRAY_MIC ${OSPRAY_BUILD_MIC_SUPPORT})
SET(OSPRAY_MPI ${OSPRAY_BUILD_MPI_DEVICE})

SET(OSPRAY_TILE_SIZE 64 CACHE INT "Tile size")
SET_PROPERTY(CACHE OSPRAY_TILE_SIZE PROPERTY STRINGS 8 16 32 64 128 256 512)

SET(OSPRAY_PIXELS_PER_JOB 64 CACHE INT
    "Must be multiple of largest vector width *and* <= OSPRAY_TILE_SIZE")

MARK_AS_ADVANCED(OSPRAY_TILE_SIZE)
MARK_AS_ADVANCED(OSPRAY_PIXELS_PER_JOB)

OSPRAY_CONFIGURE_COMPILER()
OSPRAY_CONFIGURE_TASKING_SYSTEM()

# Must be before ISA config
INCLUDE(configure_embree)
