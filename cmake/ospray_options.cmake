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
SET(OSPRAY_VERSION_MINOR 7)
SET(OSPRAY_VERSION_PATCH 2)
SET(OSPRAY_SOVERSION 0)
SET(OSPRAY_VERSION_GITHASH 0)
SET(OSPRAY_VERSION_NOTE "")
IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  FIND_PACKAGE(Git)
  IF (GIT_FOUND)
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE "OSPRAY_VERSION_GITHASH"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(SUBSTRING ${OSPRAY_VERSION_GITHASH} 0 8 OSPRAY_VERSION_GITHASH_SHORT)
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE "OSPRAY_VERSION_GITBRANCH"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    IF (NOT OSPRAY_VERSION_GITBRANCH MATCHES "^master$|^release-")
      IF (NOT OSPRAY_VERSION_GITBRANCH STREQUAL "HEAD")
        SET(OSPRAY_VERSION_NOTE "-${OSPRAY_VERSION_GITBRANCH}")
      ENDIF()
      SET(OSPRAY_VERSION_NOTE "${OSPRAY_VERSION_NOTE} (${OSPRAY_VERSION_GITHASH_SHORT})")
    ENDIF()
  ENDIF()
ENDIF()

SET(OSPRAY_VERSION
  ${OSPRAY_VERSION_MAJOR}.${OSPRAY_VERSION_MINOR}.${OSPRAY_VERSION_PATCH}
)

SET(EMBREE_VERSION_REQUIRED 3.2.0)

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

if (WIN32)
  # avoid problematic min/max defines of windows.h
  add_definitions(-DNOMINMAX)
endif()

##############################################################
# OSPRay specific build options and configuration selection
##############################################################

OSPRAY_CONFIGURE_COMPILER()
OSPRAY_CONFIGURE_TASKING_SYSTEM()

OPTION(OSPRAY_USE_EMBREE_STREAMS "Enable use of Embree's stream intersection")
MARK_AS_ADVANCED(OSPRAY_USE_EMBREE_STREAMS) # feature not implemented yet

SET(OSPRAY_TILE_SIZE 64 CACHE STRING "Tile size (x,y dimensions)")
SET_PROPERTY(CACHE OSPRAY_TILE_SIZE PROPERTY STRINGS 8 16 32 64 128 256 512)
MARK_AS_ADVANCED(OSPRAY_TILE_SIZE)

IF (WIN32)
  SET(TILE_STACK_DEFAULT_SIZE 64)
ELSEIF (APPLE)
  SET(TILE_STACK_DEFAULT_SIZE 32)
ELSE ()
  SET(TILE_STACK_DEFAULT_SIZE 128)
ENDIF()

SET(OSPRAY_MAX_STACK_TILE_SIZE ${TILE_STACK_DEFAULT_SIZE} CACHE STRING
    "Max size for tile to remain allocated on the stack")
SET_PROPERTY(CACHE OSPRAY_MAX_STACK_TILE_SIZE PROPERTY STRINGS 8 16 32 64 128 256 512)
MARK_AS_ADVANCED(OSPRAY_MAX_STACK_TILE_SIZE)

SET(OSPRAY_PIXELS_PER_JOB 64 CACHE STRING
    "Must be multiple of largest vector width *and* <= OSPRAY_TILE_SIZE")
MARK_AS_ADVANCED(OSPRAY_PIXELS_PER_JOB)


# make Embree's INSTALLs happy
INCLUDE(GNUInstallDirs)

# Must be before ISA config and package
INCLUDE(configure_embree)


OPTION(OSPRAY_ENABLE_APPS "Enable the 'apps' subtree in the build." ON)
MARK_AS_ADVANCED(OSPRAY_ENABLE_APPS)

option(OSPRAY_ENABLE_TESTING "Enable building, installing, and packaging of test tools.")
option(OSPRAY_AUTO_DOWNLOAD_TEST_IMAGES "Automatically download test images during build." ON)

IF (OSPRAY_ENABLE_TESTING)
  ENABLE_TESTING()
ENDIF()

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
  MACRO(OSPRAY_INSTALL_NAMELINK NAME TARGET_NAME)
    EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink
                    ${TARGET_NAME} ${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}.so)
    INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}.so
            DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)

    # If the shared lib we're copying is named with a specific version, also
    # create a major version suffixed symlink
    STRING(REGEX MATCH "([0-9]+)[.]([0-9]+)[.]([0-9]+)" VERSION_STRING ${TARGET_NAME})
    if (CMAKE_MATCH_0)
      EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink
                      ${TARGET_NAME}
                      ${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}.so.${CMAKE_MATCH_1})
      INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}.so.${CMAKE_MATCH_1}
              DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
    endif()
  ENDMACRO()

  IF (OSPRAY_TASKING_TBB)
    IF (WIN32)
      SET(TBB_DLL_HINTS
        HINTS
        ${TBB_ROOT}/../redist/${TBB_ARCH}_win/tbb/${TBB_VCVER}
        ${TBB_ROOT}/../redist/${TBB_ARCH}/tbb/${TBB_VCVER}
        ${TBB_ROOT}/bin/${TBB_ARCH}/${TBB_VCVER}
        ${TBB_ROOT}/bin
      )
      FIND_FILE(TBB_DLL tbb.dll ${TBB_DLL_HINTS})
      FIND_FILE(TBB_DLL_DEBUG tbb_debug.dll ${TBB_DLL_HINTS})
      FIND_FILE(TBB_DLL_MALLOC tbbmalloc.dll ${TBB_DLL_HINTS})
      FIND_FILE(TBB_DLL_MALLOC_DEBUG tbbmalloc_debug.dll ${TBB_DLL_HINTS})
      MARK_AS_ADVANCED(TBB_DLL)
      MARK_AS_ADVANCED(TBB_DLL_DEBUG)
      MARK_AS_ADVANCED(TBB_DLL_MALLOC)
      MARK_AS_ADVANCED(TBB_DLL_MALLOC_DEBUG)
      INSTALL(PROGRAMS ${TBB_DLL} ${TBB_DLL_MALLOC}
              DESTINATION ${CMAKE_INSTALL_BINDIR}
              CONFIGURATIONS Release RelWithDebInfo COMPONENT redist)
      INSTALL(PROGRAMS ${TBB_DLL_DEBUG} ${TBB_DLL_MALLOC_DEBUG}
              DESTINATION ${CMAKE_INSTALL_BINDIR}
              CONFIGURATIONS Debug COMPONENT redist)
    ELSE()
      INSTALL(PROGRAMS ${TBB_LIBRARY} ${TBB_LIBRARY_MALLOC}
              DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
      IF (NOT APPLE)
        get_filename_component(TBB_LIBNAME ${TBB_LIBRARY} NAME)
        get_filename_component(TBB_MALLOC_LIBNAME ${TBB_LIBRARY_MALLOC} NAME)
        OSPRAY_INSTALL_NAMELINK(tbb ${TBB_LIBNAME})
        OSPRAY_INSTALL_NAMELINK(tbbmalloc ${TBB_MALLOC_LIBNAME})
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
    FIND_FILE(EMBREE_DLL embree3.dll HINTS ${EMBREE_DLL_HINTS})
    MARK_AS_ADVANCED(EMBREE_DLL)
    INSTALL(PROGRAMS ${EMBREE_DLL}
            DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT redist)
  ELSE()
    INSTALL(PROGRAMS ${EMBREE_LIBRARY}
            DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
    IF (NOT APPLE)
      get_filename_component(EMBREE_LIBNAME ${EMBREE_LIBRARY} NAME)
      OSPRAY_INSTALL_NAMELINK(embree ${EMBREE_LIBNAME})
    ENDIF()
  ENDIF()
ENDIF()
