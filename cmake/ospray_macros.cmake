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

## Macro for printing CMake variables ##
MACRO(PRINT var)
  MESSAGE("${var} = ${${var}}")
ENDMACRO()

## Macro to print a warning message that only appears once ##
MACRO(OSPRAY_WARN_ONCE identifier message)
  SET(INTERNAL_WARNING "OSPRAY_WARNED_${identifier}")
  IF(NOT ${INTERNAL_WARNING})
    MESSAGE(WARNING ${message})
    SET(${INTERNAL_WARNING} ON CACHE INTERNAL "Warned about '${message}'")
  ENDIF()
ENDMACRO()

## Macro check for compiler support of ISA ##
MACRO(OSPRAY_CHECK_COMPILER_SUPPORT ISA)
  IF (${ISA} STREQUAL "AVX512")
    IF (WIN32 OR APPLE)
      OSPRAY_WARN_ONCE(MISSING_AVX512
                       "OSPRay only supports KNL on Linux. "
                       "Disabling KNL ISA target.")
      SET(OSPRAY_EMBREE_ENABLE_${ISA} false)
    ELSEIF ((NOT OSPRAY_COMPILER_ICC) AND (NOT OSPRAY_USE_EXTERNAL_EMBREE))
      OSPRAY_WARN_ONCE(MISSING_AVX512
                       "You must use ICC to compile AVX512 when using OSPRAY_USE_EXTERNAL_EMBREE=OFF. Either install and use an AVX512 compatible Embree, or switch to using the Intel Compiler.")
      SET(OSPRAY_EMBREE_ENABLE_${ISA} false)
    ENDIF()
  ELSEIF (OSPRAY_EMBREE_ENABLE_${ISA} AND NOT OSPRAY_COMPILER_SUPPORTS_${ISA})
    OSPRAY_WARN_ONCE(MISSING_${ISA}
                     "Need at least version ${GCC_VERSION_REQUIRED_${ISA}} of "
                     "GCC for ${ISA}. Disabling ${ISA}.\nTo compile for "
                     "${ISA}, please switch to either 'ICC'-compiler, or "
                     "upgrade your GCC version.")
    SET(OSPRAY_EMBREE_ENABLE_${ISA} false)
  ENDIF()
ENDMACRO()

## Macro configure ISA targets for ispc ##
MACRO(OSPRAY_CONFIGURE_ISPC_ISA)

  OSPRAY_CONFIGURE_COMPILER()

  # the arch we're targeting for the non-MIC/non-xeon phi part of ospray
  SET(OSPRAY_BUILD_ISA "ALL" CACHE STRING
      "Target ISA (SSE, AVX, AVX2, AVX512, or ALL)")
  STRING(TOUPPER ${OSPRAY_BUILD_ISA} OSPRAY_BUILD_ISA)

  SET(OSPRAY_EMBREE_ENABLE_SSE    true)
  SET(OSPRAY_EMBREE_ENABLE_AVX    true)
  SET(OSPRAY_EMBREE_ENABLE_AVX2   true)
  SET(OSPRAY_EMBREE_ENABLE_AVX512 true)

  OSPRAY_CHECK_COMPILER_SUPPORT(AVX)
  OSPRAY_CHECK_COMPILER_SUPPORT(AVX2)
  OSPRAY_CHECK_COMPILER_SUPPORT(AVX512)

  UNSET(OSPRAY_SUPPORTED_ISAS)

  IF(OSPRAY_EMBREE_ENABLE_SSE)
    SET(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} SSE)
  ENDIF()
  IF(OSPRAY_EMBREE_ENABLE_AVX AND EMBREE_ISA_SUPPORTS_AVX)
    SET(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX)
  ENDIF()
  IF(OSPRAY_EMBREE_ENABLE_AVX2 AND EMBREE_ISA_SUPPORTS_AVX2)
    SET(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX2)
  ENDIF()
  IF(OSPRAY_EMBREE_ENABLE_AVX512 AND EMBREE_ISA_SUPPORTS_AVX512)
    SET(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX512)
  ENDIF()

  SET_PROPERTY(CACHE OSPRAY_BUILD_ISA PROPERTY STRINGS
               ALL ${OSPRAY_SUPPORTED_ISAS})

  UNSET(OSPRAY_ISPC_TARGET_LIST)

  IF (OSPRAY_BUILD_ISA STREQUAL "ALL")

    IF(OSPRAY_EMBREE_ENABLE_SSE)
      SET(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} sse4)
      MESSAGE(STATUS "OSPRay SSE ISA target enabled.")
    ENDIF()
    IF(OSPRAY_EMBREE_ENABLE_AVX AND EMBREE_ISA_SUPPORTS_AVX)
      SET(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx)
      MESSAGE(STATUS "OSPRay AVX ISA target enabled.")
    ENDIF()
    IF(OSPRAY_EMBREE_ENABLE_AVX2 AND EMBREE_ISA_SUPPORTS_AVX2)
      SET(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx2)
      MESSAGE(STATUS "OSPRay AVX2 ISA target enabled.")
    ENDIF()
    IF(OSPRAY_EMBREE_ENABLE_AVX512 AND EMBREE_ISA_SUPPORTS_AVX512)
      SET(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx512knl-i32x16)
      MESSAGE(STATUS "OSPRay AVX512 ISA target enabled.")
    ENDIF()

  ELSEIF (OSPRAY_BUILD_ISA STREQUAL "AVX512")

    IF(NOT OSPRAY_EMBREE_ENABLE_AVX512)
      MESSAGE(FATAL_ERROR "Compiler does not support AVX512!")
    ELSEIF(NOT EMBREE_ISA_SUPPORTS_AVX512)
      MESSAGE(FATAL_ERROR
              "Your Embree build only supports up to ${${EMBREE_ISA_NAME}}!")
    ENDIF()
    SET(OSPRAY_ISPC_TARGET_LIST avx512knl-i32x16)

  ELSEIF (OSPRAY_BUILD_ISA STREQUAL "AVX2")

    IF(NOT OSPRAY_EMBREE_ENABLE_AVX2)
      MESSAGE(FATAL_ERROR "Compiler does not support AVX2!")
    ELSEIF(NOT EMBREE_ISA_SUPPORTS_AVX2)
      MESSAGE(FATAL_ERROR
              "Your Embree build only supports up to ${${EMBREE_ISA_NAME}}!")
    ENDIF()

    SET(OSPRAY_ISPC_TARGET_LIST avx2)
    SET(OSPRAY_EMBREE_ENABLE_AVX512 false)

  ELSEIF (OSPRAY_BUILD_ISA STREQUAL "AVX")

    IF(NOT OSPRAY_EMBREE_ENABLE_AVX)
      MESSAGE(FATAL_ERROR "Compiler does not support AVX!")
    ELSEIF(NOT EMBREE_ISA_SUPPORTS_AVX)
      MESSAGE(FATAL_ERROR
              "Your Embree build only supports up to ${${EMBREE_ISA_NAME}}!")
    ENDIF()

    SET(OSPRAY_ISPC_TARGET_LIST avx)
    SET(OSPRAY_EMBREE_ENABLE_AVX512 false)
    SET(OSPRAY_EMBREE_ENABLE_AVX2   false)

  ELSEIF (OSPRAY_BUILD_ISA STREQUAL "SSE")
    SET(OSPRAY_ISPC_TARGET_LIST sse4)
    SET(OSPRAY_EMBREE_ENABLE_AVX512 false)
    SET(OSPRAY_EMBREE_ENABLE_AVX2   false)
    SET(OSPRAY_EMBREE_ENABLE_AVX    false)
  ELSE ()
    MESSAGE(ERROR "Invalid OSPRAY_BUILD_ISA value. "
                  "Please select one of SSE, AVX, AVX2, AVX512, or ALL.")
  ENDIF()
ENDMACRO()

## Target creation macros ##

MACRO(OSPRAY_ADD_SUBDIRECTORY subdirectory)
  SET(OSPRAY_EXE_SUFFIX "")
  SET(OSPRAY_LIB_SUFFIX "")
  SET(OSPRAY_ISPC_SUFFIX ".o")
  SET(THIS_IS_MIC OFF)
  SET(__XEON__ ON)

  ADD_SUBDIRECTORY(${subdirectory} builddir/${subdirectory}/intel64)

  IF (OSPRAY_MIC)
    SET(OSPRAY_EXE_SUFFIX ".mic")
    SET(OSPRAY_LIB_SUFFIX "_mic")
    SET(OSPRAY_ISPC_SUFFIX ".cpp")
    SET(OSPRAY_ISPC_TARGET "mic")
    SET(THIS_IS_MIC ON)
    SET(__XEON__ OFF)
    INCLUDE(icc_xeonphi)
    SET(OSPRAY_TARGET_MIC ON PARENT_SCOPE)

    ADD_SUBDIRECTORY(${subdirectory} builddir/${subdirectory}/mic)
  ENDIF()
ENDMACRO()

MACRO(OSPRAY_ADD_EXECUTABLE name)
  ADD_EXECUTABLE(${name}${OSPRAY_EXE_SUFFIX} ${ARGN})
ENDMACRO()

MACRO(OSPRAY_ADD_LIBRARY name type)
  SET(ISPC_SOURCES "")
  SET(OTHER_SOURCES "")
  FOREACH(src ${ARGN})
    GET_FILENAME_COMPONENT(ext ${src} EXT)
    IF (ext STREQUAL ".ispc")
      SET(ISPC_SOURCES ${ISPC_SOURCES} ${src})
    ELSE ()
      SET(OTHER_SOURCES ${OTHER_SOURCES} ${src})
    ENDIF ()
  ENDFOREACH()
  OSPRAY_ISPC_COMPILE(${ISPC_SOURCES})
  ADD_LIBRARY(${name}${OSPRAY_LIB_SUFFIX} ${type}
              ${ISPC_OBJECTS} ${OTHER_SOURCES} ${ISPC_SOURCES})

  IF (THIS_IS_MIC)
    FOREACH(src ${ISPC_OBJECTS})
      SET_SOURCE_FILES_PROPERTIES(${src} PROPERTIES COMPILE_FLAGS -std=gnu++98)
    ENDFOREACH()
  ENDIF()
ENDMACRO()

## Target linking macros ##

MACRO(OSPRAY_TARGET_LINK_LIBRARIES name)
  SET(LINK_LIBS "")

  IF(THIS_IS_MIC)
    FOREACH(lib ${ARGN})
      STRING(LENGTH ${lib} lib_length)
      IF (${lib_length} GREATER 5)
        STRING(SUBSTRING ${lib} 0 6 lib_begin)
      ENDIF ()
      IF (${lib_length} GREATER 5 AND ":${lib_begin}" STREQUAL ":ospray")
        LIST(APPEND LINK_LIBS ${lib}${OSPRAY_LIB_SUFFIX})
      ELSE ()
        LIST(APPEND LINK_LIBS ${lib})
      ENDIF ()
    ENDFOREACH()
  ELSE()
    SET(LINK_LIBS ${ARGN})
  ENDIF()

  TARGET_LINK_LIBRARIES(${name} ${LINK_LIBS})
ENDMACRO()

MACRO(OSPRAY_LIBRARY_LINK_LIBRARIES name)
  OSPRAY_TARGET_LINK_LIBRARIES(${name}${OSPRAY_LIB_SUFFIX} ${ARGN})
ENDMACRO()

MACRO(OSPRAY_EXE_LINK_LIBRARIES name)
  OSPRAY_TARGET_LINK_LIBRARIES(${name}${OSPRAY_EXE_SUFFIX} ${ARGN})
ENDMACRO()

## Target install macros for OSPRay libraries ##
# use vanilla INSTALL for apps -- these don't have MIC parts and should also not
# go into COMPONENT lib

MACRO(OSPRAY_INSTALL_LIBRARY name)
  # NOTE(jda) - Check if CMAKE_INSTALL_LIB/BINDIR is set, and use
  #             CMAKE_INSTALL_PREFIX if it's not defined. It may not be defined
  #             in a client project who calls this macro in their CMakeLists.
  IF(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    SET(LOCAL_LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
  ELSE()
    SET(LOCAL_LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
  ENDIF()

  IF(NOT DEFINED CMAKE_INSTALL_BINDIR)
    SET(LOCAL_BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
  ELSE()
    SET(LOCAL_BIN_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
  ENDIF()

  INSTALL(TARGETS ${name}${OSPRAY_LIB_SUFFIX} ${ARGN}
    LIBRARY DESTINATION ${LOCAL_LIB_INSTALL_DIR}
      COMPONENT lib${OSPRAY_LIB_SUFFIX}
    # on Windows put the dlls into bin
    RUNTIME DESTINATION ${LOCAL_BIN_INSTALL_DIR}
      COMPONENT lib
    # ... and the import lib into the devel package
    ARCHIVE DESTINATION ${LOCAL_LIB_INSTALL_DIR}
      COMPONENT devel
  )
ENDMACRO()

# only use for executables required for OSPRay functionality, e.g. the MPI
# worker, because these go into install component 'lib'
MACRO(OSPRAY_INSTALL_EXE _name)
  SET(name ${_name}${OSPRAY_EXE_SUFFIX})
  # use OSPRAY_LIB_SUFFIX for COMPONENT to get lib_mic and not lib.mic
  INSTALL(TARGETS ${name} ${ARGN} 
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT lib${OSPRAY_LIB_SUFFIX}
  )
ENDMACRO()

## Target versioning macro ##

MACRO(OSPRAY_SET_LIBRARY_VERSION _name)
  SET(name ${_name}${OSPRAY_LIB_SUFFIX})
  SET_TARGET_PROPERTIES(${name}
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
ENDMACRO()

## Conveniance macro for creating OSPRay libraries ##
# Usage
#
#   OSPRAY_CREATE_LIBRARY(<name> source1 [source2 ...]
#                         [LINK lib1 [lib2 ...]])
#
# will create and install shared library <name> from 'sources' with
# version OSPRAY_[SO]VERSION and optionally link against 'libs'

MACRO(OSPRAY_CREATE_LIBRARY LIBRARY_NAME)
  SET(LIBRARY_SOURCES "")
  SET(LINK_LIBS "")

  SET(CURRENT_LIST LIBRARY_SOURCES)
  FOREACH(arg ${ARGN})
    IF (${arg} STREQUAL "LINK")
      SET(CURRENT_LIST LINK_LIBS)
    ELSE()
      LIST(APPEND ${CURRENT_LIST} ${arg})
    ENDIF ()
  ENDFOREACH()
 
  OSPRAY_ADD_LIBRARY(${LIBRARY_NAME} SHARED ${LIBRARY_SOURCES})
  OSPRAY_LIBRARY_LINK_LIBRARIES(${LIBRARY_NAME} ${LINK_LIBS})
  OSPRAY_SET_LIBRARY_VERSION(${LIBRARY_NAME})
  OSPRAY_INSTALL_LIBRARY(${LIBRARY_NAME})
ENDMACRO()

## Conveniance macro for creating OSPRay applications ##
# Usage
#
#   OSPRAY_CREATE_APPLICATION(<name> source1 [source2 ...]
#                             [LINK lib1 [lib2 ...]])
#
# will create and install application <name> from 'sources' with version
# OSPRAY_VERSION and optionally link against 'libs'

MACRO(OSPRAY_CREATE_APPLICATION APP_NAME)
  SET(APP_SOURCES "")
  SET(LINK_LIBS "")

  SET(CURRENT_LIST APP_SOURCES)
  FOREACH(arg ${ARGN})
    IF (${arg} STREQUAL "LINK")
      SET(CURRENT_LIST LINK_LIBS)
    ELSE()
      LIST(APPEND ${CURRENT_LIST} ${arg})
    ENDIF ()
  ENDFOREACH()
 
  ADD_EXECUTABLE(${APP_NAME} ${APP_SOURCES})
  TARGET_LINK_LIBRARIES(${APP_NAME} ${LINK_LIBS})
  IF (WIN32)
    SET_TARGET_PROPERTIES(${APP_NAME} PROPERTIES VERSION ${OSPRAY_VERSION})
  ENDIF()
  INSTALL(TARGETS ${APP_NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT apps
  )
ENDMACRO()

## Conveniance macro for installing OSPRay headers ##
# Usage
#
#   OSPRAY_INSTALL_SDK_HEADERS(header1 [header2 ...] [DESTINATION destination])
#
# will install headers into ${CMAKE_INSTALL_PREFIX}/ospray/SDK/${destination},
# where destination is optional.

MACRO(OSPRAY_INSTALL_SDK_HEADERS)
  SET(HEADERS "")
  SET(DESTINATION "")

  SET(CURRENT_LIST HEADERS)
  FOREACH(arg ${ARGN})
    IF (${arg} STREQUAL "DESTINATION")
      SET(CURRENT_LIST DESTINATION)
    ELSE()
      LIST(APPEND ${CURRENT_LIST} ${arg})
    ENDIF ()
  ENDFOREACH()

  INSTALL(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK/${DESTINATION}
    COMPONENT devel
  )
ENDMACRO()

## Compiler configuration macro ##

MACRO(OSPRAY_CONFIGURE_COMPILER)
  # unhide compiler to make it easier for users to see what they are using
  MARK_AS_ADVANCED(CLEAR CMAKE_CXX_COMPILER)

  SET(OSPRAY_COMPILER_ICC   OFF)
  SET(OSPRAY_COMPILER_GCC   OFF)
  SET(OSPRAY_COMPILER_CLANG OFF)
  SET(OSPRAY_COMPILER_MSVC  OFF)

  IF (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    SET(OSPRAY_COMPILER_ICC ON)
    INCLUDE(icc)
  ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    SET(OSPRAY_COMPILER_GCC ON)
    INCLUDE(gcc)
  ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    SET(OSPRAY_COMPILER_CLANG ON)
    INCLUDE(clang)
  ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    SET(OSPRAY_COMPILER_MSVC ON)
    INCLUDE(msvc)
  ELSE()
    MESSAGE(FATAL_ERROR
            "Unsupported compiler specified: '${CMAKE_CXX_COMPILER_ID}'")
  ENDIF()
ENDMACRO()

## Tasking system configuration macro ##

MACRO(OSPRAY_CONFIGURE_TASKING_SYSTEM)
  # -------------------------------------------------------
  # Setup tasking system build configuration
  # -------------------------------------------------------

  # NOTE(jda) - Notice that this implies that OSPRAY_CONFIGURE_COMPILER() has
  #             been called before this macro!
  IF(OSPRAY_COMPILER_ICC)
    SET(CILK_STRING "Cilk")
  ENDIF()

  # NOTE(jda) - Always default to TBB, at least until Cilk is *exactly* the same
  #             as TBB...
  #IF(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
  #  SET(TASKING_DEFAULT ${CILK_STRING})
  #ELSE()
    SET(TASKING_DEFAULT TBB)
  #ENDIF()

  SET(OSPRAY_TASKING_SYSTEM ${TASKING_DEFAULT} CACHE STRING
      "Per-node thread tasking system [TBB,OpenMP,Cilk,Internal,Debug]")

  SET_PROPERTY(CACHE OSPRAY_TASKING_SYSTEM PROPERTY
               STRINGS TBB ${CILK_STRING} OpenMP Internal Debug)
  MARK_AS_ADVANCED(OSPRAY_TASKING_SYSTEM)

  # NOTE(jda) - Make the OSPRAY_TASKING_SYSTEM build option case-insensitive
  STRING(TOUPPER ${OSPRAY_TASKING_SYSTEM} OSPRAY_TASKING_SYSTEM_ID)

  SET(OSPRAY_TASKING_TBB      FALSE)
  SET(OSPRAY_TASKING_CILK     FALSE)
  SET(OSPRAY_TASKING_OPENMP   FALSE)
  SET(OSPRAY_TASKING_INTERNAL FALSE)
  SET(OSPRAY_TASKING_DEBUG    FALSE)

  IF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "TBB")
    SET(OSPRAY_TASKING_TBB TRUE)
  ELSEIF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "CILK")
    SET(OSPRAY_TASKING_CILK TRUE)
  ELSEIF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "OPENMP")
    SET(OSPRAY_TASKING_OPENMP TRUE)
  ELSEIF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "INTERNAL")
    SET(OSPRAY_TASKING_INTERNAL TRUE)
  ELSE()
    SET(OSPRAY_TASKING_DEBUG TRUE)
  ENDIF()

  UNSET(TASKING_SYSTEM_LIBS)
  UNSET(TASKING_SYSTEM_LIBS_MIC)

  IF(OSPRAY_TASKING_TBB)
    FIND_PACKAGE(TBB REQUIRED)
    ADD_DEFINITIONS(-DOSPRAY_TASKING_TBB)
    INCLUDE_DIRECTORIES(${TBB_INCLUDE_DIRS})
    SET(TASKING_SYSTEM_LIBS ${TBB_LIBRARIES})
    SET(TASKING_SYSTEM_LIBS_MIC ${TBB_LIBRARIES_MIC})
  ELSE(OSPRAY_TASKING_TBB)
    UNSET(TBB_INCLUDE_DIR          CACHE)
    UNSET(TBB_LIBRARY              CACHE)
    UNSET(TBB_LIBRARY_DEBUG        CACHE)
    UNSET(TBB_LIBRARY_MALLOC       CACHE)
    UNSET(TBB_LIBRARY_MALLOC_DEBUG CACHE)
    UNSET(TBB_INCLUDE_DIR_MIC      CACHE)
    UNSET(TBB_LIBRARY_MIC          CACHE)
    UNSET(TBB_LIBRARY_MALLOC_MIC   CACHE)
    IF(OSPRAY_TASKING_OPENMP)
      FIND_PACKAGE(OpenMP)
      IF (OPENMP_FOUND)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        SET(CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
        ADD_DEFINITIONS(-DOSPRAY_TASKING_OMP)
      ENDIF()
    ELSEIF(OSPRAY_TASKING_CILK)
      ADD_DEFINITIONS(-DOSPRAY_TASKING_CILK)
      IF (OSPRAY_COMPILER_GCC OR OSPRAY_COMPILER_CLANG)
        OSPRAY_WARN_ONCE(UNSAFE_USE_OF_CILK
            "You are using Cilk with GCC or Clang...use at your own risk!")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcilkplus")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcilkplus")
      ENDIF()
    ELSEIF(OSPRAY_TASKING_INTERNAL)
      ADD_DEFINITIONS(-DOSPRAY_TASKING_INTERNAL)
    ELSE()#Debug
      # Do nothing, will fall back to scalar code (useful for debugging)
    ENDIF()
  ENDIF(OSPRAY_TASKING_TBB)
ENDMACRO()
