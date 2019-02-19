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

##############################################################
# Global configuration options
##############################################################

set(OSPRAY_CMAKECONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/ospray-${OSPRAY_VERSION}")

set(EMBREE_VERSION_REQUIRED 3.2.0)

set(CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
if (WIN32)
  if (NOT OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET)
    set(CMAKE_CONFIGURATION_TYPES "${CONFIGURATION_TYPES}"
        CACHE STRING "List of generated configurations." FORCE)
    set(OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET ON
        CACHE INTERNAL "Default CMake configuration types set.")
  endif()
else()
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type." FORCE)
  endif()
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(OSPRAY_BUILD_DEBUG          TRUE )
  set(OSPRAY_BUILD_RELWITHDEBINFO FALSE)
  set(OSPRAY_BUILD_RELEASE        FALSE)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
  set(OSPRAY_BUILD_DEBUG          FALSE)
  set(OSPRAY_BUILD_RELWITHDEBINFO TRUE )
  set(OSPRAY_BUILD_RELEASE        FALSE)
else()# Release
  set(OSPRAY_BUILD_DEBUG          FALSE)
  set(OSPRAY_BUILD_RELWITHDEBINFO FALSE)
  set(OSPRAY_BUILD_RELEASE        TRUE )
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

if (WIN32)
  # avoid problematic min/max defines of windows.h
  add_definitions(-DNOMINMAX)
endif()

##############################################################
# OSPRay specific build options and configuration selection
##############################################################

ospray_configure_compiler()
ospray_configure_tasking_system()
ospray_create_tasking_target()

option(OSPRAY_USE_EMBREE_STREAMS "Enable use of Embree's stream intersection")
mark_as_advanced(OSPRAY_USE_EMBREE_STREAMS) # feature not implemented yet

set(OSPRAY_TILE_SIZE 64 CACHE STRING "Tile size (x,y dimensions)")
set_property(CACHE OSPRAY_TILE_SIZE PROPERTY STRINGS 8 16 32 64 128 256 512)
mark_as_advanced(OSPRAY_TILE_SIZE)

if (WIN32)
  set(TILE_STACK_DEFAULT_SIZE 64)
elseif (APPLE)
  set(TILE_STACK_DEFAULT_SIZE 32)
else ()
  set(TILE_STACK_DEFAULT_SIZE 128)
endif()

set(OSPRAY_MAX_STACK_TILE_SIZE ${TILE_STACK_DEFAULT_SIZE} CACHE STRING
    "Max size for tile to remain allocated on the stack")
set_property(CACHE OSPRAY_MAX_STACK_TILE_SIZE PROPERTY STRINGS 8 16 32 64 128 256 512)
mark_as_advanced(OSPRAY_MAX_STACK_TILE_SIZE)

set(OSPRAY_PIXELS_PER_JOB 64 CACHE STRING
    "Must be multiple of largest vector width *and* <= OSPRAY_TILE_SIZE")
mark_as_advanced(OSPRAY_PIXELS_PER_JOB)

# Must be before ISA config and package
include(configure_embree)

option(OSPRAY_ENABLE_TUTORIALS "Enable the 'tutorials' subtree in the build." ON)

option(OSPRAY_ENABLE_APPS "Enable the 'apps' subtree in the build." ON)
mark_as_advanced(OSPRAY_ENABLE_APPS)

option(OSPRAY_ENABLE_TESTING "Enable building, installing, and packaging of test tools.")
option(OSPRAY_AUTO_DOWNLOAD_TEST_IMAGES "Automatically download test images during build." ON)

if (OSPRAY_ENABLE_TESTING)
  enable_testing()
endif()

##############################################################
# create binary packages; before any install() invocation/definition
##############################################################

option(OSPRAY_ZIP_MODE "Use tarball/zip CPack generator instead of RPM" ON)
mark_as_advanced(OSPRAY_ZIP_MODE)

option(OSPRAY_INSTALL_DEPENDENCIES "Install OSPRay dependencies in binary packages and install")
mark_as_advanced(OSPRAY_INSTALL_DEPENDENCIES)

include(package)

##############################################################
# redistribute TBB and Embree
##############################################################

if (OSPRAY_INSTALL_DEPENDENCIES)
  macro(OSPRAY_INSTALL_NAMELINK NAME TARGET_NAME)
    set(BASE_LIB_NAME lib${NAME})
    set(LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
    execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
                    ${TARGET_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${BASE_LIB_NAME}${LIB_SUFFIX})
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}${LIB_SUFFIX}
            DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)

    # If the shared lib we're copying is named with a specific version, also
    # create a major version suffixed symlink
    string(REGEX MATCH "([0-9]+)[.]([0-9]+)[.]([0-9]+)" VERSION_STRING ${TARGET_NAME})
    if (CMAKE_MATCH_0)
      if(APPLE)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
                        ${TARGET_NAME}
                        ${CMAKE_CURRENT_BINARY_DIR}/${BASE_LIB_NAME}.${CMAKE_MATCH_1}${LIB_SUFFIX})
        install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${BASE_LIB_NAME}.${CMAKE_MATCH_1}${LIB_SUFFIX}
                DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
      else()
        execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
                        ${TARGET_NAME}
                        ${CMAKE_CURRENT_BINARY_DIR}/${BASE_LIB_NAME}${LIB_SUFFIX}.${CMAKE_MATCH_1})
        install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${BASE_LIB_NAME}${LIB_SUFFIX}.${CMAKE_MATCH_1}
                DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
      endif()
    endif()
  endmacro()

  if (OSPRAY_TASKING_TBB)
    if (WIN32)
      set(TBB_DLL_HINTS
        HINTS
        ${TBB_ROOT}/../redist/${TBB_ARCH}_win/tbb/vc14
        ${TBB_ROOT}/../redist/${TBB_ARCH}/tbb/vc14
        ${TBB_ROOT}/bin/${TBB_ARCH}/vc14
        ${TBB_ROOT}/bin
      )
      find_file(TBB_DLL tbb.dll ${TBB_DLL_HINTS})
      find_file(TBB_DLL_DEBUG tbb_debug.dll ${TBB_DLL_HINTS})
      find_file(TBB_DLL_MALLOC tbbmalloc.dll ${TBB_DLL_HINTS})
      find_file(TBB_DLL_MALLOC_DEBUG tbbmalloc_debug.dll ${TBB_DLL_HINTS})
      mark_as_advanced(TBB_DLL)
      mark_as_advanced(TBB_DLL_DEBUG)
      mark_as_advanced(TBB_DLL_MALLOC)
      mark_as_advanced(TBB_DLL_MALLOC_DEBUG)
      install(PROGRAMS ${TBB_DLL} ${TBB_DLL_MALLOC}
              DESTINATION ${CMAKE_INSTALL_BINDIR}
              CONFIGURATIONS Release RelWithDebInfo COMPONENT redist)
      install(PROGRAMS ${TBB_DLL_DEBUG} ${TBB_DLL_MALLOC_DEBUG}
              DESTINATION ${CMAKE_INSTALL_BINDIR}
              CONFIGURATIONS Debug COMPONENT redist)
    else()
      install(PROGRAMS ${TBB_LIBRARY} ${TBB_LIBRARY_MALLOC}
              DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
      if (NOT APPLE)
        get_filename_component(TBB_LIBNAME ${TBB_LIBRARY} NAME)
        get_filename_component(TBB_MALLOC_LIBNAME ${TBB_LIBRARY_MALLOC} NAME)
        ospray_install_namelink(tbb ${TBB_LIBNAME})
        ospray_install_namelink(tbbmalloc ${TBB_MALLOC_LIBNAME})
      endif()
    endif()
  endif()

  if (WIN32)
    get_filename_component(EMBREE_LIB_DIR ${EMBREE_LIBRARY} PATH)
    set(EMBREE_DLL_HINTS
      ${EMBREE_LIB_DIR}
      ${EMBREE_LIB_DIR}/../bin
      ${embree_DIR}/../../../bin
      ${embree_DIR}/../bin
    )
    find_file(EMBREE_DLL embree3.dll HINTS ${EMBREE_DLL_HINTS})
    mark_as_advanced(EMBREE_DLL)
    install(PROGRAMS ${EMBREE_DLL}
            DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT redist)
  else()
    install(PROGRAMS ${EMBREE_LIBRARY}
            DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT redist)
    if (NOT APPLE)
      get_filename_component(EMBREE_LIBNAME ${EMBREE_LIBRARY} NAME)
      ospray_install_namelink(embree ${EMBREE_LIBNAME})
    endif()
  endif()
endif()
