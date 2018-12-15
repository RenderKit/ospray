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


# workaround link issues to Embree ISPC exports
# ISPC only adds the ISA suffix during name mangling (and dynamic dispatch
# code) when compiling for multiple targets. Thus, when only one OSPRay ISA is
# selected, but Embree was compiled for multiple ISAs, we need to add a
# second, different, supported dummy target.
MACRO(OSPRAY_FIX_ISPC_TARGET_LIST)
  LIST(LENGTH OSPRAY_ISPC_TARGET_LIST NUM_TARGETS)
  IF (NUM_TARGETS EQUAL 1)
    IF (EMBREE_ISA_SUPPORTS_SSE2)
      LIST(APPEND OSPRAY_ISPC_TARGET_LIST sse2)
    ELSEIF (EMBREE_ISA_SUPPORTS_SSE4 AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "sse4")
      LIST(APPEND OSPRAY_ISPC_TARGET_LIST sse4)
    ELSEIF (EMBREE_ISA_SUPPORTS_AVX AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "avx")
      LIST(APPEND OSPRAY_ISPC_TARGET_LIST avx)
    ELSEIF (EMBREE_ISA_SUPPORTS_AVX2 AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "avx2")
      LIST(APPEND OSPRAY_ISPC_TARGET_LIST avx2)
    ELSEIF (EMBREE_ISA_SUPPORTS_AVX512KNL AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "avx512knl-i32x16")
      LIST(APPEND OSPRAY_ISPC_TARGET_LIST avx512knl-i32x16)
    ELSEIF (EMBREE_ISA_SUPPORTS_AVX512SKX AND
            NOT OSPRAY_ISPC_TARGET_LIST STREQUAL "avx512skx-i32x16")
      LIST(APPEND OSPRAY_ISPC_TARGET_LIST avx512skx-i32x16)
    ENDIF()
  ENDIF()
ENDMACRO()

## Macro configure ISA targets for ispc ##
MACRO(OSPRAY_CONFIGURE_ISPC_ISA)

  SET(OSPRAY_BUILD_ISA "ALL" CACHE STRING
      "Target ISA (SSE4, AVX, AVX2, AVX512KNL, AVX512SKX, or ALL)")
  STRING(TOUPPER ${OSPRAY_BUILD_ISA} OSPRAY_BUILD_ISA)

  OPTION(OSPRAY_BUILD_ISA_SCALAR
         "Include 'SCALAR' target (WARNING: may not work!)" OFF)
  MARK_AS_ADVANCED(OSPRAY_BUILD_ISA_SCALAR)

  IF (OSPRAY_BUILD_ISA_SCALAR)
    SET(OSPRAY_SUPPORTED_ISAS SCALAR)
  ENDIF()

  IF(EMBREE_ISA_SUPPORTS_SSE4)
    SET(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} SSE4)
  ENDIF()
  IF(EMBREE_ISA_SUPPORTS_AVX)
    SET(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX)
  ENDIF()
  IF(EMBREE_ISA_SUPPORTS_AVX2)
    SET(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX2)
  ENDIF()
  IF(EMBREE_ISA_SUPPORTS_AVX512KNL)
    SET(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX512KNL)
  ENDIF()
  IF(EMBREE_ISA_SUPPORTS_AVX512SKX)
    SET(OSPRAY_SUPPORTED_ISAS ${OSPRAY_SUPPORTED_ISAS} AVX512SKX)
  ENDIF()

  SET_PROPERTY(CACHE OSPRAY_BUILD_ISA PROPERTY STRINGS
               ALL ${OSPRAY_SUPPORTED_ISAS})

  UNSET(OSPRAY_ISPC_TARGET_LIST)

  IF (OSPRAY_BUILD_ISA STREQUAL "ALL")

    IF(EMBREE_ISA_SUPPORTS_SSE4)
      SET(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} sse4)
      MESSAGE(STATUS "OSPRay SSE4 ISA target enabled.")
    ENDIF()
    IF(EMBREE_ISA_SUPPORTS_AVX)
      SET(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx)
      MESSAGE(STATUS "OSPRay AVX ISA target enabled.")
    ENDIF()
    IF(EMBREE_ISA_SUPPORTS_AVX2)
      SET(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx2)
      MESSAGE(STATUS "OSPRay AVX2 ISA target enabled.")
    ENDIF()
    IF(EMBREE_ISA_SUPPORTS_AVX512KNL)
      SET(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx512knl-i32x16)
      MESSAGE(STATUS "OSPRay AVX512KNL ISA target enabled.")
    ENDIF()
    IF(EMBREE_ISA_SUPPORTS_AVX512SKX)
      SET(OSPRAY_ISPC_TARGET_LIST ${OSPRAY_ISPC_TARGET_LIST} avx512skx-i32x16)
      MESSAGE(STATUS "OSPRay AVX512SKX ISA target enabled.")
    ENDIF()

  ELSEIF (OSPRAY_BUILD_ISA STREQUAL "AVX512SKX")

    IF(NOT EMBREE_ISA_SUPPORTS_AVX512SKX)
      MESSAGE(FATAL_ERROR "Your Embree build does not support AVX512SKX!")
    ENDIF()
    SET(OSPRAY_ISPC_TARGET_LIST avx512skx-i32x16)

  ELSEIF (OSPRAY_BUILD_ISA STREQUAL "AVX512KNL")

    IF(NOT EMBREE_ISA_SUPPORTS_AVX512KNL)
      MESSAGE(FATAL_ERROR "Your Embree build does not support AVX512KNL!")
    ENDIF()
    SET(OSPRAY_ISPC_TARGET_LIST avx512knl-i32x16)

  ELSEIF (OSPRAY_BUILD_ISA STREQUAL "AVX2")

    IF(NOT EMBREE_ISA_SUPPORTS_AVX2)
      MESSAGE(FATAL_ERROR "Your Embree build does not support AVX2!")
    ENDIF()
    SET(OSPRAY_ISPC_TARGET_LIST avx2)

  ELSEIF (OSPRAY_BUILD_ISA STREQUAL "AVX")

    IF(NOT EMBREE_ISA_SUPPORTS_AVX)
      MESSAGE(FATAL_ERROR "Your Embree build does not support AVX!")
    ENDIF()
    SET(OSPRAY_ISPC_TARGET_LIST avx)

  ELSEIF (OSPRAY_BUILD_ISA STREQUAL "SSE4")

    IF(NOT EMBREE_ISA_SUPPORTS_SSE4)
      MESSAGE(FATAL_ERROR "Your Embree build does not support SSE4!")
    ENDIF()
    SET(OSPRAY_ISPC_TARGET_LIST sse4)

  ELSE ()
    MESSAGE(ERROR "Invalid OSPRAY_BUILD_ISA value. "
                  "Please select one of ${OSPRAY_SUPPORTED_ISAS}, or ALL.")
  ENDIF()

  OSPRAY_FIX_ISPC_TARGET_LIST()
ENDMACRO()

## Target creation macros ##

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
  ADD_LIBRARY(${name} ${type} ${ISPC_OBJECTS} ${OTHER_SOURCES} ${ISPC_SOURCES})
ENDMACRO()

## Target install macros for OSPRay libraries ##
INCLUDE(GNUInstallDirs)

MACRO(OSPRAY_INSTALL_LIBRARY name component)
  INSTALL(TARGETS ${name}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT ${component}
      NAMELINK_SKIP
    # on Windows put the dlls into bin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT ${component}
    # ... and the import lib into the devel package
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT devel
  )

  # Install the namelink in the devel component. This command also includes the
  # RUNTIME and ARCHIVE components a second time to prevent an "install TARGETS
  # given no ARCHIVE DESTINATION for static library target" error. Installing
  # these components twice doesn't hurt anything.
  INSTALL(TARGETS ${name}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT devel
      NAMELINK_ONLY
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT ${component}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT devel
  )
ENDMACRO()

## Target versioning macro ##

MACRO(OSPRAY_SET_LIBRARY_VERSION name)
  SET_TARGET_PROPERTIES(${name}
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
ENDMACRO()


# Helper function to return arguments of OSPRAY_CREATE_ in separate
# variables, prefixed with PREFIX
FUNCTION(OSPRAY_SPLIT_CREATE_ARGS PREFIX)
  SET(MY_SOURCES "")
  SET(LIBS "")
  SET(MY_EXCLUDE_FROM_ALL FALSE)
  SET(MY_COMPONENT ${OSPRAY_DEFAULT_COMPONENT})
  SET(CURRENT_LIST MY_SOURCES)

  FOREACH(arg ${ARGN})
    IF ("${arg}" STREQUAL "LINK")
      SET(CURRENT_LIST LIBS)
    ELSEIF ("${arg}" STREQUAL "EXCLUDE_FROM_ALL")
      SET(MY_EXCLUDE_FROM_ALL TRUE)
    ELSEIF ("${arg}" STREQUAL "COMPONENT")
      SET(CURRENT_LIST MY_COMPONENT)
    ELSE()
      LIST(APPEND ${CURRENT_LIST} ${arg})
    ENDIF ()
  ENDFOREACH()

  # COMPONENT only required when installed
  IF (NOT ${MY_EXCLUDE_FROM_ALL})
    LIST(LENGTH MY_COMPONENT COMPONENTS)
    IF (COMPONENTS EQUAL 0)
      MESSAGE(STATUS "No COMPONENT for installation specified! Using default COMPONENT name \"${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME}\"")
      SET(MY_COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME})
    ELSE()
      LIST(GET MY_COMPONENT -1 MY_COMPONENT)
    ENDIF()
  ENDIF()

  SET(${PREFIX}_SOURCES ${MY_SOURCES} PARENT_SCOPE)
  SET(${PREFIX}_LIBS ${LIBS} PARENT_SCOPE)
  SET(${PREFIX}_EXCLUDE_FROM_ALL ${MY_EXCLUDE_FROM_ALL} PARENT_SCOPE)
  SET(${PREFIX}_COMPONENT ${MY_COMPONENT} PARENT_SCOPE)
ENDFUNCTION()

## Conveniance macro for creating OSPRay libraries ##
# Usage
#
#   OSPRAY_CREATE_LIBRARY(<name> source1 [source2 ...]
#                         [LINK lib1 [lib2 ...]]
#                         [EXCLUDE_FROM_ALL]
#                         [COMPONENT component])
#
# will create and install shared library <name> from 'sources' with
# version OSPRAY_[SO]VERSION and optionally link against 'libs'.
# When EXCLUDE_FROM_ALL is given the library will not be installed and
# only be build when it is an explicit dependency.
# Per default the application will be installed in component
# OSPRAY_DEFAULT_COMPONENT which can be overridden individually with
# COMPONENT.

MACRO(OSPRAY_CREATE_LIBRARY LIBRARY_NAME)
  OSPRAY_SPLIT_CREATE_ARGS(LIBRARY ${ARGN})

  OSPRAY_ADD_LIBRARY(${LIBRARY_NAME} SHARED ${LIBRARY_SOURCES})
  TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${LIBRARY_LIBS})
  OSPRAY_SET_LIBRARY_VERSION(${LIBRARY_NAME})
  IF(${LIBRARY_EXCLUDE_FROM_ALL})
    SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
  ELSE()
    OSPRAY_INSTALL_LIBRARY(${LIBRARY_NAME} ${LIBRARY_COMPONENT})
  ENDIF()
ENDMACRO()

## Conveniance macro for creating OSPRay applications ##
# Usage
#
#   OSPRAY_CREATE_APPLICATION(<name> source1 [source2 ...]
#                             [LINK lib1 [lib2 ...]]
#                             [EXCLUDE_FROM_ALL]
#                             [COMPONENT component])
#
# will create and install application <name> from 'sources' with version
# OSPRAY_VERSION and optionally link against 'libs'.
# When EXCLUDE_FROM_ALL is given the application will not be installed
# and only be build when it is an explicit dependency.
# Per default the application will be installed in component
# OSPRAY_DEFAULT_COMPONENT which can be overridden individually with
# COMPONENT.

MACRO(OSPRAY_CREATE_APPLICATION APP_NAME)
  OSPRAY_SPLIT_CREATE_ARGS(APP ${ARGN})

  ADD_EXECUTABLE(${APP_NAME} ${APP_SOURCES})
  TARGET_LINK_LIBRARIES(${APP_NAME} ${APP_LIBS})
  IF (WIN32)
    SET_TARGET_PROPERTIES(${APP_NAME} PROPERTIES VERSION ${OSPRAY_VERSION})
  ENDIF()
  IF(${APP_EXCLUDE_FROM_ALL})
    SET_TARGET_PROPERTIES(${APP_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
  ELSE()
    INSTALL(TARGETS ${APP_NAME}
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT ${APP_COMPONENT}
    )
  ENDIF()
ENDMACRO()

## Conveniance macro for creating OSPRay test applications ##
# Usage - same as OSPRAY_CREATE_APPLICATION
#
# Will build the app if 'OSPRAY_ENABLE_TESTING=ON' in CMake

MACRO(OSPRAY_CREATE_TEST)
  IF (OSPRAY_ENABLE_TESTING)
    OSPRAY_CREATE_APPLICATION(
      ${ARGN}
      COMPONENT test
    )
  ENDIF()
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
  SET(MY_DESTINATION "")

  SET(CURRENT_LIST HEADERS)
  FOREACH(arg ${ARGN})
    IF ("${arg}" STREQUAL "DESTINATION")
      SET(CURRENT_LIST MY_DESTINATION)
    ELSE()
      LIST(APPEND ${CURRENT_LIST} ${arg})
    ENDIF ()
  ENDFOREACH()

  INSTALL(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK/${MY_DESTINATION}
    COMPONENT devel
  )
ENDMACRO()

## Compiler configuration macros ##

MACRO(OSPRAY_CONFIGURE_COMPILER)
  # unhide compiler to make it easier for users to see what they are using
  MARK_AS_ADVANCED(CLEAR CMAKE_CXX_COMPILER)

  OPTION(OSPRAY_STRICT_BUILD "Build with additional warning flags" ON)
  MARK_AS_ADVANCED(OSPRAY_STRICT_BUILD)

  OPTION(OSPRAY_WARN_AS_ERRORS "Treat warnings as errors" OFF)
  MARK_AS_ADVANCED(OSPRAY_WARN_AS_ERRORS)

  SET(OSPRAY_COMPILER_ICC   FALSE)
  SET(OSPRAY_COMPILER_GCC   FALSE)
  SET(OSPRAY_COMPILER_CLANG FALSE)
  SET(OSPRAY_COMPILER_MSVC  FALSE)

  IF (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    SET(OSPRAY_COMPILER_ICC TRUE)
    IF(WIN32) # icc on Windows behaves like msvc
      INCLUDE(msvc)
    ELSE()
      INCLUDE(icc)
    ENDIF()
  ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    SET(OSPRAY_COMPILER_GCC TRUE)
    INCLUDE(gcc)
  ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    SET(OSPRAY_COMPILER_CLANG TRUE)
    INCLUDE(clang)
  ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    SET(OSPRAY_COMPILER_MSVC TRUE)
    INCLUDE(msvc)
  ELSE()
    MESSAGE(FATAL_ERROR
            "Unsupported compiler specified: '${CMAKE_CXX_COMPILER_ID}'")
  ENDIF()

  IF (WIN32)
    # increase stack to 8MB (the default size of 1MB is too small for our apps)
    # note: linker options are independent of compiler (icc or MSVC)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8388608")
  ENDIF()
ENDMACRO()

MACRO(OSPRAY_DISABLE_COMPILER_WARNINGS)
  IF (NOT OSPRAY_COMPILER_MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
  #ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
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

  SET(OSPRAY_TASKING_SYSTEM TBB CACHE STRING
      "Per-node thread tasking system [TBB,OpenMP,Cilk,LibDispatch,Internal,Debug]")

  SET_PROPERTY(CACHE OSPRAY_TASKING_SYSTEM PROPERTY
               STRINGS TBB ${CILK_STRING} OpenMP Internal LibDispatch Debug)
  MARK_AS_ADVANCED(OSPRAY_TASKING_SYSTEM)

  # NOTE(jda) - Make the OSPRAY_TASKING_SYSTEM build option case-insensitive
  STRING(TOUPPER ${OSPRAY_TASKING_SYSTEM} OSPRAY_TASKING_SYSTEM_ID)

  SET(OSPRAY_TASKING_TBB         FALSE)
  SET(OSPRAY_TASKING_CILK        FALSE)
  SET(OSPRAY_TASKING_OPENMP      FALSE)
  SET(OSPRAY_TASKING_INTERNAL    FALSE)
  SET(OSPRAY_TASKING_LIBDISPATCH FALSE)
  SET(OSPRAY_TASKING_DEBUG       FALSE)

  IF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "TBB")
    SET(OSPRAY_TASKING_TBB TRUE)
  ELSEIF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "CILK")
    SET(OSPRAY_TASKING_CILK TRUE)
  ELSEIF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "OPENMP")
    SET(OSPRAY_TASKING_OPENMP TRUE)
  ELSEIF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "INTERNAL")
    SET(OSPRAY_TASKING_INTERNAL TRUE)
  ELSEIF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "LIBDISPATCH")
    SET(OSPRAY_TASKING_LIBDISPATCH TRUE)
  ELSE()
    SET(OSPRAY_TASKING_DEBUG TRUE)
  ENDIF()

  UNSET(TASKING_SYSTEM_LIBS)

  IF(OSPRAY_TASKING_TBB)
    FIND_PACKAGE(TBB REQUIRED)
    ADD_DEFINITIONS(-DOSPRAY_TASKING_TBB)
    INCLUDE_DIRECTORIES(${TBB_INCLUDE_DIRS})
    SET(TASKING_SYSTEM_LIBS ${TBB_LIBRARIES})
  ELSE(OSPRAY_TASKING_TBB)
    UNSET(TBB_INCLUDE_DIR          CACHE)
    UNSET(TBB_LIBRARY              CACHE)
    UNSET(TBB_LIBRARY_DEBUG        CACHE)
    UNSET(TBB_LIBRARY_MALLOC       CACHE)
    UNSET(TBB_LIBRARY_MALLOC_DEBUG CACHE)
    IF(OSPRAY_TASKING_OPENMP)
      FIND_PACKAGE(OpenMP)
      IF (OPENMP_FOUND)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        IF(OSPRAY_COMPILER_ICC) # workaround linker issue #115
          SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -liomp5")
        ENDIF()
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
    ELSEIF(OSPRAY_TASKING_LIBDISPATCH)
      FIND_PACKAGE(libdispatch REQUIRED)
      INCLUDE_DIRECTORIES(${LIBDISPATCH_INCLUDE_DIRS})
      SET(TASKING_SYSTEM_LIBS ${LIBDISPATCH_LIBRARIES})
      ADD_DEFINITIONS(-DOSPRAY_TASKING_LIBDISPATCH)
    ELSE()#Debug
      # Do nothing, will fall back to scalar code (useful for debugging)
    ENDIF()
  ENDIF(OSPRAY_TASKING_TBB)
ENDMACRO()

## MPI configuration macro ##

MACRO(OSPRAY_CONFIGURE_MPI)
  IF (WIN32) # FindMPI does not find Intel MPI on Windows, we need to help here
    FIND_PACKAGE(MPI)

    # need to strip quotes, otherwise CMake treats it as relative path
    STRING(REGEX REPLACE "^\"|\"$" "" MPI_CXX_INCLUDE_PATH ${MPI_CXX_INCLUDE_PATH})

    IF (NOT MPI_CXX_FOUND)
      # try again, hinting the compiler wrappers
      SET(MPI_CXX_COMPILER mpicxx.bat)
      SET(MPI_C_COMPILER mpicc.bat)
      FIND_PACKAGE(MPI)

      IF (NOT MPI_CXX_LIBRARIES)
        SET(MPI_LIB_PATH ${MPI_CXX_INCLUDE_PATH}\\..\\lib)

        SET(MPI_LIB "MPI_LIB-NOTFOUND" CACHE FILEPATH "Cleared" FORCE)
        FIND_LIBRARY(MPI_LIB NAMES impi HINTS ${MPI_LIB_PATH})
        SET(MPI_C_LIB ${MPI_LIB})
        SET(MPI_C_LIBRARIES ${MPI_LIB} CACHE STRING "MPI C libraries to link against" FORCE)

        SET(MPI_LIB "MPI_LIB-NOTFOUND" CACHE FILEPATH "Cleared" FORCE)
        FIND_LIBRARY(MPI_LIB NAMES impicxx HINTS ${MPI_LIB_PATH})
        SET(MPI_CXX_LIBRARIES ${MPI_C_LIB} ${MPI_LIB} CACHE STRING "MPI CXX libraries to link against" FORCE)
        SET(MPI_LIB "MPI_LIB-NOTFOUND" CACHE INTERNAL "Scratch variable for MPI lib detection" FORCE)
      ENDIF()
    ENDIF()
  ELSE()
    FIND_PACKAGE(MPI REQUIRED)
  ENDIF()

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MPI_CXX_LINK_FLAGS}")

  INCLUDE_DIRECTORIES(SYSTEM ${MPI_CXX_INCLUDE_PATH})
ENDMACRO()

# Keep backwards compatible CONFIGURE_MPI() macro, but warn of deprecation
MACRO(CONFIGURE_MPI)
  OSPRAY_WARN_ONCE(CONFIGURE_MPI_DEPRECATION
                   "CONFIGURE_MPI() is deprecated, use new OSPRAY_CONFIGURE_MPI().")
  OSPRAY_CONFIGURE_MPI()
ENDMACRO()
