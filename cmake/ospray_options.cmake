## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
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
SET(OSPRAY_VERSION_MINOR 2)
SET(OSPRAY_VERSION_PATCH 1)
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
  ENDIF()
  SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})
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
# OSPRay specific build options and configuration selection
##############################################################

OSPRAY_CONFIGURE_COMPILER()
OSPRAY_CONFIGURE_TASKING_SYSTEM()

OPTION(OSPRAY_USE_EMBREE_STREAMS "Enable use of Embree's stream intersection")
MARK_AS_ADVANCED(OSPRAY_USE_EMBREE_STREAMS) # feature not implemented yet

SET(OSPRAY_TILE_SIZE 64 CACHE STRING "Tile size")
SET_PROPERTY(CACHE OSPRAY_TILE_SIZE PROPERTY STRINGS 8 16 32 64 128 256 512)
MARK_AS_ADVANCED(OSPRAY_TILE_SIZE)

SET(OSPRAY_PIXELS_PER_JOB 64 CACHE STRING
    "Must be multiple of largest vector width *and* <= OSPRAY_TILE_SIZE")
MARK_AS_ADVANCED(OSPRAY_PIXELS_PER_JOB)


# make Embree's INSTALLs happy
INCLUDE(GNUInstallDirs)

# Must be before ISA config and package
INCLUDE(configure_embree)

##############################################################
# create binary packages; before any INSTALL() invocation/definition
##############################################################

OPTION(OSPRAY_ZIP_MODE "Use tarball/zip CPack generator instead of RPM" ON)
MARK_AS_ADVANCED(OSPRAY_ZIP_MODE)

OPTION(OSPRAY_INSTALL_DEPENDENCIES "Install OSPRay dependencies in binary packages and install")
MARK_AS_ADVANCED(OSPRAY_INSTALL_DEPENDENCIES)

INCLUDE(package)

IF (OSPRAY_INSTALL_DEPENDENCIES)
  IF (WIN32)
    GET_FILENAME_COMPONENT(EMBREE_LIB_DIR ${EMBREE_LIBRARY} PATH)
    SET(EMBREE_DLL_HINTS
      ${EMBREE_LIB_DIR}
      ${EMBREE_LIB_DIR}/../bin
      ${embree_DIR}/../../../bin
      ${embree_DIR}/../bin
    )
    FIND_FILE(EMBREE_DLL embree.dll HINTS ${EMBREE_DLL_HINTS})
    MARK_AS_ADVANCED(EMBREE_DLL)
    INSTALL(PROGRAMS ${EMBREE_DLL}
            DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT redist)
  ELSE()
    INSTALL(PROGRAMS ${EMBREE_LIBRARY}
            DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
  ENDIF()
ENDIF()
