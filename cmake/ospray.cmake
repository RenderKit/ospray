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

#include bindir - that's where ispc puts generated header files
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
SET(OSPRAY_BINARY_DIR ${CMAKE_BINARY_DIR})
SET(OSPRAY_DIR ${PROJECT_SOURCE_DIR})
# arch-specific cmd-line flags for various arch and compiler configs

SET(OSPRAY_TILE_SIZE 64 CACHE INT "Tile size")
SET(OSPRAY_PIXELS_PER_JOB 64 CACHE INT
    "Must be multiple of largest vector width *and* <= OSPRAY_TILE_SIZE")

MARK_AS_ADVANCED(OSPRAY_TILE_SIZE)
MARK_AS_ADVANCED(OSPRAY_PIXELS_PER_JOB)

# unhide compiler to make it easier for users to see what they are using
MARK_AS_ADVANCED(CLEAR CMAKE_CXX_COMPILER)

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
  IF (OSPRAY_EMBREE_ENABLE_${ISA} AND NOT OSPRAY_COMPILER_SUPPORTS_${ISA})
    OSPRAY_WARN_ONCE(MISSING_${ISA} "Need at least version ${GCC_VERSION_REQUIRED_${ISA}} of gcc for ${ISA}. Disabling ${ISA}.\nTo compile for ${ISA}, please switch to either 'ICC'-compiler, or upgrade your gcc version.")
    SET(OSPRAY_EMBREE_ENABLE_${ISA} false)
  ENDIF()
ENDMACRO()


# Configure the output directories. To allow IMPI to do its magic we
# will put *executables* into the (same) build directory, but tag
# mic-executables with ".mic". *libraries* cannot use the
# ".mic"-suffix trick, so we'll put libraries into separate
# directories (names 'intel64' and 'mic', respectively)
MACRO(CONFIGURE_OSPRAY)
  CONFIGURE_TASKING_SYSTEM()

  IF (OSPRAY_TARGET STREQUAL "mic")
    SET(OSPRAY_EXE_SUFFIX ".mic")
    SET(OSPRAY_LIB_SUFFIX "_mic")
    SET(OSPRAY_ISPC_SUFFIX ".cpp")
    SET(OSPRAY_ISPC_TARGET "mic")
    SET(THIS_IS_MIC ON)
    SET(__XEON__ OFF)
    INCLUDE(${PROJECT_SOURCE_DIR}/cmake/icc_xeonphi.cmake)

    SET(OSPRAY_TARGET_MIC ON PARENT_SCOPE)
  ELSE()
    SET(OSPRAY_EXE_SUFFIX "")
    SET(OSPRAY_LIB_SUFFIX "")
    SET(OSPRAY_ISPC_SUFFIX ".o")
    SET(THIS_IS_MIC OFF)
    SET(__XEON__ ON)
    IF (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
      INCLUDE(${PROJECT_SOURCE_DIR}/cmake/icc.cmake)
    ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
      INCLUDE(${PROJECT_SOURCE_DIR}/cmake/gcc.cmake)
    ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
      INCLUDE(${PROJECT_SOURCE_DIR}/cmake/clang.cmake)
    ELSEIF (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
      INCLUDE(${PROJECT_SOURCE_DIR}/cmake/msvc.cmake)
    ELSE()
      MESSAGE(FATAL_ERROR "Unsupported compiler specified: '${CMAKE_CXX_COMPILER_ID}'")
    ENDIF()

    SET(OSPRAY_EMBREE_ENABLE_AVX512 false)
    IF (OSPRAY_BUILD_ISA STREQUAL "ALL")
      # ------------------------------------------------------------------
      # in 'all' mode, we have a list of multiple targets for ispc,
      # and enable all targets for embree (we may still disable some
      # below if the compiler doesn't support them
      # ------------------------------------------------------------------
      SET(OSPRAY_ISPC_TARGET_LIST sse4 avx avx2)
      SET(OSPRAY_EMBREE_ENABLE_SSE  true)
      SET(OSPRAY_EMBREE_ENABLE_AVX  true)
      SET(OSPRAY_EMBREE_ENABLE_AVX2 true)
      IF (OSPRAY_BUILD_ENABLE_KNL)
        SET(OSPRAY_EMBREE_ENABLE_AVX512 true)
        SET(OSPRAY_ISPC_TARGET_LIST sse4 avx avx2 avx512knl-i32x16)
      ENDIF()

    ELSEIF (OSPRAY_BUILD_ISA STREQUAL "AVX512")
      # ------------------------------------------------------------------
      # in 'avx512' mode, we currently build only avx512, in generic
      # mode, but enable all embree targets to fall back to (currently
      # does not work since embree would require a 16-wide trace
      # function which it has in neither of the three targets)
      # ------------------------------------------------------------------
      SET(OSPRAY_ISPC_TARGET_LIST avx512knl-i32x16)
      SET(OSPRAY_EMBREE_ENABLE_SSE  true)
      SET(OSPRAY_EMBREE_ENABLE_AVX  true)
      SET(OSPRAY_EMBREE_ENABLE_AVX2 true)
      SET(OSPRAY_EMBREE_ENABLE_AVX512 true)

    ELSEIF (OSPRAY_BUILD_ISA STREQUAL "AVX2")
      # ------------------------------------------------------------------
      # in 'avx2' mode, we enable ONLY avx2 for ispc, and all targets
      # up to avx2 for embree. note that if the compiler doesn't
      # support AVX we will have a combination where embree uses AVX
      # (the most the compiler can do), while ispc still uses
      # avx. this works because both targets are 8 wide. it does
      # however require the compiler to understand AT LEAST AVX1.
      # ------------------------------------------------------------------
      SET(OSPRAY_ISPC_TARGET_LIST avx2)
      SET(OSPRAY_EMBREE_ENABLE_SSE  true)
      SET(OSPRAY_EMBREE_ENABLE_AVX  true)
      SET(OSPRAY_EMBREE_ENABLE_AVX2 true)

    ELSEIF (OSPRAY_BUILD_ISA STREQUAL "AVX")
      # ------------------------------------------------------------------
      # in 'avx' mode, we enable ONLY avx for ispc, and both sse and
      # avx for embree. note that this works ONLY works if the
      # compiler knows at least AVX
      # ------------------------------------------------------------------
      SET(OSPRAY_ISPC_TARGET_LIST avx)
      SET(OSPRAY_EMBREE_ENABLE_SSE  true)
      SET(OSPRAY_EMBREE_ENABLE_AVX  true)
      SET(OSPRAY_EMBREE_ENABLE_AVX2 false)

    ELSEIF (OSPRAY_BUILD_ISA STREQUAL "SSE")
      # ------------------------------------------------------------------
      # in 'sse' mode, we enable ONLY sse4 for ispc, and only sse for
      # embree
      # ------------------------------------------------------------------
      SET(OSPRAY_ISPC_TARGET_LIST sse4)
      SET(OSPRAY_EMBREE_ENABLE_SSE  true)
      SET(OSPRAY_EMBREE_ENABLE_AVX  false)
      SET(OSPRAY_EMBREE_ENABLE_AVX2 false)
    ELSE ()
      MESSAGE(ERROR "Invalid OSPRAY_BUILD_ISA value. Please select one of SSE, AVX, AVX2, or ALL.")
    ENDIF()

  ENDIF()

  OSPRAY_CHECK_COMPILER_SUPPORT(AVX)
  OSPRAY_CHECK_COMPILER_SUPPORT(AVX2)
  OSPRAY_CHECK_COMPILER_SUPPORT(AVX512)

  IF (THIS_IS_MIC)
    OPTION(OSPRAY_BUILD_COI_DEVICE "Build COI Device for OSPRay's MIC support?" ON)
  ENDIF()

  INCLUDE(${PROJECT_SOURCE_DIR}/cmake/ispc.cmake)

  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray/include)

  # for auto-generated cmakeconfig etc
  INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_BINARY_DIR})

ENDMACRO()

## Target creation macros ##

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
  ADD_LIBRARY(${name}${OSPRAY_LIB_SUFFIX} ${type} ${ISPC_OBJECTS} ${OTHER_SOURCES} ${ISPC_SOURCES})

  IF (THIS_IS_MIC)
    FOREACH(src ${ISPC_OBJECTS})
      SET_SOURCE_FILES_PROPERTIES( ${src} PROPERTIES COMPILE_FLAGS -std=gnu++98 )
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
  INSTALL(TARGETS ${name}${OSPRAY_LIB_SUFFIX} ${ARGN}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT lib${OSPRAY_LIB_SUFFIX}
    # on Windows put the dlls into bin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT lib
    # ... and the import lib into the devel package
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT devel
  )
ENDMACRO()

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

## Compiler configuration macro ##

MACRO(OSPRAY_CONFIGURE_COMPILER)
  # enable ability for users to force a compiler using the pre-0.8.3 method
  SET(OSPRAY_COMPILER "" CACHE STRING "Force compiler: GCC, ICC, CLANG")
  SET_PROPERTY(CACHE OSPRAY_COMPILER PROPERTY STRINGS GCC ICC CLANG)
  MARK_AS_ADVANCED(OSPRAY_COMPILER)

  IF(NOT ":${OSPRAY_COMPILER}" STREQUAL ":")
    STRING(TOUPPER ${OSPRAY_COMPILER} OSPRAY_COMPILER)
    IF(${OSPRAY_COMPILER} STREQUAL "GCC")
      FIND_PROGRAM(GCC_EXECUTABLE gcc DOC "Path to the gcc executable.")
      FIND_PROGRAM(G++_EXECUTABLE g++ DOC "Path to the g++ executable.")
      SET(CMAKE_C_COMPILER ${GCC_EXECUTABLE} CACHE STRING "C Compiler" FORCE)
      SET(CMAKE_CXX_COMPILER ${G++_EXECUTABLE} CACHE STRING "CXX Compiler" FORCE)
      SET(CMAKE_C_COMPILER "gcc")
      SET(CMAKE_CXX_COMPILER "g++")
      SET(CMAKE_CXX_COMPILER_ID "GNU")
    ELSEIF(${OSPRAY_COMPILER} STREQUAL "ICC")
      FIND_PROGRAM(ICC_EXECUTABLE icc DOC "Path to the icc executable.")
      FIND_PROGRAM(ICPC_EXECUTABLE icpc DOC "Path to the icpc executable.")
      SET(CMAKE_C_COMPILER ${ICC_EXECUTABLE} CACHE STRING "CXX Compiler" FORCE)
      SET(CMAKE_CXX_COMPILER ${ICPC_EXECUTABLE} CACHE STRING "CXX Compiler" FORCE)
      SET(CMAKE_CXX_COMPILER_ID "Intel")
    ELSEIF(${OSPRAY_COMPILER} STREQUAL "CLANG")
      FIND_PROGRAM(CLANG_EXECUTABLE clang DOC "Path to the clang executable.")
      FIND_PROGRAM(CLANG_EXECUTABLE clang++ DOC "Path to the clang++ executable.")
      SET(CMAKE_C_COMPILER ${CLANG_EXECUTABLE} CACHE STRING "C Compiler" FORCE)
      SET(CMAKE_CXX_COMPILER ${CLANG_EXECUTABLE} CACHE STRING "CXX Compiler" FORCE)
      SET(CMAKE_CXX_COMPILER_ID "Clang")
    ENDIF()
  ENDIF()
ENDMACRO()

## Tasking system configuration macro ##

MACRO(CONFIGURE_TASKING_SYSTEM)
  # -------------------------------------------------------
  # Setup tasking system build configuration
  # -------------------------------------------------------

  IF(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
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

  SET(USE_TBB      FALSE)
  SET(USE_CILK     FALSE)
  SET(USE_OPENMP   FALSE)
  SET(USE_INTERNAL FALSE)

  IF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "TBB")
    SET(USE_TBB TRUE)
  ELSEIF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "CILK")
    SET(USE_CILK TRUE)
  ELSEIF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "OPENMP")
    SET(USE_OPENMP TRUE)
  ELSEIF(${OSPRAY_TASKING_SYSTEM_ID} STREQUAL "INTERNAL")
    SET(USE_INTERNAL TRUE)
  ENDIF()

  UNSET(TASKING_SYSTEM_LIBS)
  UNSET(TASKING_SYSTEM_LIBS_MIC)

  IF(USE_TBB)
    FIND_PACKAGE(TBB REQUIRED)
    ADD_DEFINITIONS(-DOSPRAY_USE_TBB)
    INCLUDE_DIRECTORIES(${TBB_INCLUDE_DIRS})
    SET(TASKING_SYSTEM_LIBS ${TBB_LIBRARIES})
    SET(TASKING_SYSTEM_LIBS_MIC ${TBB_LIBRARIES_MIC})
  ELSE(USE_TBB)
    UNSET(TBB_INCLUDE_DIR          CACHE)
    UNSET(TBB_LIBRARY              CACHE)
    UNSET(TBB_LIBRARY_DEBUG        CACHE)
    UNSET(TBB_LIBRARY_MALLOC       CACHE)
    UNSET(TBB_LIBRARY_MALLOC_DEBUG CACHE)
    UNSET(TBB_INCLUDE_DIR_MIC      CACHE)
    UNSET(TBB_LIBRARY_MIC          CACHE)
    UNSET(TBB_LIBRARY_MALLOC_MIC   CACHE)
    IF(USE_OPENMP)
      FIND_PACKAGE(OpenMP)
      IF (OPENMP_FOUND)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        SET(CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
        ADD_DEFINITIONS(-DOSPRAY_USE_OMP)
      ENDIF()
    ELSEIF(USE_CILK)
      ADD_DEFINITIONS(-DOSPRAY_USE_CILK)
    ELSEIF(USE_INTERNAL)
      ADD_DEFINITIONS(-DOSPRAY_USE_INTERNAL_TASKING)
    ELSE()#Debug
      # Do nothing, will fall back to scalar code (useful for debugging)
    ENDIF()
  ENDIF(USE_TBB)
ENDMACRO()
