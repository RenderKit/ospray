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

# Configure the output directories. To allow IMPI to do its magic we
# will put *executables* into the (same) build directory, but tag
# mic-executables with ".mic". *libraries* cannot use the
# ".mic"-suffix trick, so we'll put libraries into separate
# directories (names 'intel64' and 'mic', respectively)
MACRO(CONFIGURE_OSPRAY)
  # Embree common include directories; others may be added depending on build target.
  # this section could be sooo much cleaner if embree only used
  # fully-qualified include names...
  SET(EMBREE_INCLUDE_DIRECTORIES
    ${OSPRAY_EMBREE_SOURCE_DIR}/
    ${OSPRAY_EMBREE_SOURCE_DIR}/include
    ${OSPRAY_EMBREE_SOURCE_DIR}/common
    ${OSPRAY_EMBREE_SOURCE_DIR}/
    ${OSPRAY_EMBREE_SOURCE_DIR}/kernels
    )

  IF (OSPRAY_TARGET STREQUAL "mic")
    SET(OSPRAY_EXE_SUFFIX ".mic")
    SET(OSPRAY_LIB_SUFFIX "_mic")
    SET(OSPRAY_ISPC_SUFFIX ".cpp")
    SET(OSPRAY_ISPC_TARGET "mic")
    SET(THIS_IS_MIC ON)
    SET(__XEON__ OFF)
    INCLUDE(${PROJECT_SOURCE_DIR}/cmake/icc_xeonphi.cmake)

    # additional Embree include directory
    LIST(APPEND EMBREE_INCLUDE_DIRECTORIES ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeonphi)

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

    # additional Embree include directory
    LIST(APPEND EMBREE_INCLUDE_DIRECTORIES ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon)

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

  IF (OSPRAY_EMBREE_ENABLE_AVX AND NOT OSPRAY_COMPILER_SUPPORTS_AVX)
    IF (NOT OSPRAY_WARNED_MISSING_AVX)
      MESSAGE("Warning: Need at least version ${GCC_VERSION_REQUIRED_AVX} of gcc for AVX. Disabling AVX.\nTo compile for AVX, please switch to either 'ICC'-compiler, or upgrade your gcc version.")
      SET(OSPRAY_WARNED_MISSING_AVX ON CACHE INTERNAL "Warned about missing AVX support.")
    ENDIF()
    SET(OSPRAY_EMBREE_ENABLE_AVX false)
  ENDIF()

  IF (OSPRAY_EMBREE_ENABLE_AVX2 AND NOT OSPRAY_COMPILER_SUPPORTS_AVX2)
    IF (NOT OSPRAY_WARNED_MISSING_AVX2)
      MESSAGE("Warning: Need at least version ${GCC_VERSION_REQUIRED_AVX2} of gcc for AVX2. Disabling AVX2.\nTo compile for AVX2, please switch to either 'ICC'-compiler, or upgrade your gcc version.")
      SET(OSPRAY_WARNED_MISSING_AVX2 ON CACHE INTERNAL "Warned about missing AVX2 support.")
    ENDIF()
    SET(OSPRAY_EMBREE_ENABLE_AVX2 false)
  ENDIF()

  IF (OSPRAY_EMBREE_ENABLE_AVX512 AND NOT OSPRAY_COMPILER_SUPPORTS_AVX512)
    IF (NOT OSPRAY_WARNED_MISSING_AVX2)
      MESSAGE("Warning: Need at least version ${GCC_VERSION_REQUIRED_AVX512} of gcc for AVX512. Disabling AVX512.\nTo compile for AVX512, please switch to either 'ICC'-compiler, or upgrade your gcc version.")
      SET(OSPRAY_WARNED_MISSING_AVX512 ON CACHE INTERNAL "Warned about missing AVX512 support.")
    ENDIF()
    SET(OSPRAY_EMBREE_ENABLE_AVX512 false)
  ENDIF()

  IF (THIS_IS_MIC)
    # whether to build in MIC/xeon phi support
    SET(OSPRAY_BUILD_COI_DEVICE OFF CACHE BOOL "Build COI Device for OSPRay's MIC support?")
  ENDIF()

  INCLUDE(${PROJECT_SOURCE_DIR}/cmake/ispc.cmake)

  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
  INCLUDE_DIRECTORIES(${EMBREE_INCLUDE_DIRECTORIES})

  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
  INCLUDE_DIRECTORIES_ISPC(${EMBREE_INCLUDE_DIRECTORIES})

  # for auto-generated cmakeconfig etc
  INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_BINARY_DIR})

ENDMACRO()


## Target creation macros ##

MACRO(OSPRAY_ADD_EXECUTABLE _name)
  SET(name ${_name}${OSPRAY_EXE_SUFFIX})
  ADD_EXECUTABLE(${name} ${ARGN})
ENDMACRO()

MACRO(OSPRAY_ADD_LIBRARY _name type)
  SET(name ${_name}${OSPRAY_LIB_SUFFIX})
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
      IF (${lib_length} GREATER 5 AND ${lib_begin} STREQUAL "ospray")
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

MACRO(OSPRAY_LIBRARY_LINK_LIBRARIES _name)
  SET(name ${_name}${OSPRAY_LIB_SUFFIX})
  OSPRAY_TARGET_LINK_LIBRARIES(${name} ${ARGN})
ENDMACRO()

MACRO(OSPRAY_EXE_LINK_LIBRARIES _name)
  SET(name ${_name}${OSPRAY_EXE_SUFFIX})
  OSPRAY_TARGET_LINK_LIBRARIES(${name} ${ARGN})
ENDMACRO()

## Target install macros for OSPRay libraries ##
# use vanilla INSTALL for apps -- these don't have MIC parts and should also not
# go into COMPONENT lib

MACRO(OSPRAY_INSTALL_LIBRARY _name)
  SET(name ${_name}${OSPRAY_LIB_SUFFIX})
  INSTALL(TARGETS ${name} ${ARGN}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT lib${OSPRAY_LIB_SUFFIX}
  )
  IF(WIN32) # on Windows put the ospray.dll also into the apps package
    INSTALL(TARGETS ${name} ${ARGN}
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT apps
    )
  ENDIF()
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
