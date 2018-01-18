## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
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
SET(OSPRAY_VERSION_MINOR 5)
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

SET(EMBREE_VERSION_REQUIRED 2.15.0)

OPTION(OSPRAY_ENABLE_TESTING OFF)

IF (OSPRAY_ENABLE_TESTING)
  ENABLE_TESTING()
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
  SET(OSPRAY_BUILD_DEBUG          TRUE )
  SET(OSPRAY_BUILD_RELWITHDEBINFO FALSE)
  SET(OSPRAY_BUILD_RELEASE        FALSE)
ELSEIF("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
  SET(OSPRAY_BUILD_DEBUG          FALSE)
  SET(OSPRAY_BUILD_RELWITHDEBINFO TRUE )
  SET(OSPRAY_BUILD_RELEASE        FALSE)
ELSE()# Release
  SET(OSPRAY_BUILD_DEBUG          FALSE)
  SET(OSPRAY_BUILD_RELWITHDEBINFO FALSE)
  SET(OSPRAY_BUILD_RELEASE        TRUE )
ENDIF()

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

#include bindir - that's where OSPConfig.h and ospray/version.h will be put
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})

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

##############################################################
# redistribute TBB and Embree
##############################################################

IF (OSPRAY_INSTALL_DEPENDENCIES)
  MACRO(OSPRAY_INSTALL_NAMELINK NAME)
    EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink
                    lib${NAME}.so.2 ${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}.so)
    INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}.so
            DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
  ENDMACRO()

  IF (OSPRAY_TASKING_TBB)
    IF (WIN32)
      SET(TBB_DLL_HINTS
        ${TBB_ROOT}/../redist/${TBB_ARCH}_win/tbb/${TBB_VCVER}
        ${TBB_ROOT}/../redist/${TBB_ARCH}/tbb/${TBB_VCVER}
        ${TBB_ROOT}/bin/${TBB_ARCH}/${TBB_VCVER}
        ${TBB_ROOT}/bin
      )
      FIND_FILE(TBB_DLL tbb.dll HINTS ${TBB_DLL_HINTS})
      FIND_FILE(TBB_DLL_MALLOC tbbmalloc.dll PATHS HINTS ${TBB_DLL_HINTS})
      MARK_AS_ADVANCED(TBB_DLL)
      MARK_AS_ADVANCED(TBB_DLL_MALLOC)
      INSTALL(PROGRAMS ${TBB_DLL} ${TBB_DLL_MALLOC}
              DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT redist)
    ELSE()
      INSTALL(PROGRAMS ${TBB_LIBRARY} ${TBB_LIBRARY_MALLOC}
              DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
      IF (NOT APPLE)
        OSPRAY_INSTALL_NAMELINK(tbb)
        OSPRAY_INSTALL_NAMELINK(tbbmalloc)
      ENDIF()
    ENDIF()
  ENDIF()

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
    IF (NOT APPLE)
      OSPRAY_INSTALL_NAMELINK(embree)
    ENDIF()
  ENDIF()
ENDIF()
