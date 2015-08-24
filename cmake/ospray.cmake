## ======================================================================== ##
## Copyright 2009-2015 Intel Corporation                                    ##
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

FILE(WRITE "${CMAKE_BINARY_DIR}/CMakeDefines.h" "#define CMAKE_BUILD_DIR \"${CMAKE_BINARY_DIR}\"\n")

#include bindir - that's where ispc puts generated header files
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
SET(OSPRAY_BINARY_DIR ${CMAKE_BINARY_DIR})
SET(OSPRAY_DIR ${PROJECT_SOURCE_DIR})
# arch-specific cmd-line flags for various arch and compiler configs

# Configure the output directories. To allow IMPI to do its magic we
# will put *executables* into the (same) build directory, but tag
# mic-executables with ".mic". *libraries* cannot use the
# ".mic"-suffix trick, so we'll put libraries into separate
# directories (names 'intel64' and 'mic', respectively)
MACRO(CONFIGURE_OSPRAY_NO_ARCH)
  SET(LIBRARY_OUTPUT_PATH ${OSPRAY_BINARY_DIR})
  SET(EXECUTABLE_OUTPUT_PATH ${OSPRAY_BINARY_DIR})

  LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})

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

    #		SET(LIBRARY_OUTPUT_PATH "${OSPRAY_BINARY_DIR}/lib/mic")
    ADD_DEFINITIONS(-DOSPRAY_TARGET_MIC=1)
  ELSE()
    SET(OSPRAY_EXE_SUFFIX "")
    SET(OSPRAY_LIB_SUFFIX "")
    SET(OSPRAY_ISPC_SUFFIX ".o")
    SET(THIS_IS_MIC OFF)
    SET(__XEON__ ON)
    IF (WIN32)
      INCLUDE(${PROJECT_SOURCE_DIR}/cmake/msvc.cmake)
    ELSE()
      IF ((OSPRAY_COMPILER STREQUAL "ICC"))
        INCLUDE(${PROJECT_SOURCE_DIR}/cmake/icc.cmake)
      ELSEIF ((OSPRAY_COMPILER STREQUAL "GCC"))
        INCLUDE(${PROJECT_SOURCE_DIR}/cmake/gcc.cmake)
      ELSEIF ((OSPRAY_COMPILER STREQUAL "CLANG"))
        INCLUDE(${PROJECT_SOURCE_DIR}/cmake/clang.cmake)
      ELSE()
        MESSAGE(FATAL_ERROR "Unknown OSPRAY_COMPILER '${OSPRAY_COMPILER}'; recognized values are 'clang', 'icc', and 'gcc'")
      ENDIF()
    ENDIF()

    # additional Embree include directory
    LIST(APPEND EMBREE_INCLUDE_DIRECTORIES ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon)

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

    ELSEIF (OSPRAY_BUILD_ISA STREQUAL "AVX512")
      # ------------------------------------------------------------------
      # in 'avx512' mode, we currently build only avx512, in generic
      # mode, but enable all embree targets to fall back to (currently
      # does not work since embree would require a 16-wide trace
      # function which it has in neither of the three targets)
      # ------------------------------------------------------------------
			IF (OSPRAY_ISPC_KNL_NATIVE)
				SET(OSPRAY_ISPC_TARGET_LIST knl-avx512)
			ELSE()
				SET(OSPRAY_ISPC_TARGET_LIST generic-16)
			ENDIF()
      SET(OSPRAY_EMBREE_ENABLE_SSE  true)
      SET(OSPRAY_EMBREE_ENABLE_AVX  true)
      SET(OSPRAY_EMBREE_ENABLE_AVX2 true)
      # add this flag to tell embree to offer a rtcIntersect16 that actually does two rtcIntersect8's
      ADD_DEFINITIONS(-D__EMBREE_KNL_WORKAROUND__=1)
      ADD_DEFINITIONS(-DEMBREE_AVX512_WORKAROUND=1)

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

  IF (OSPRAY_MPI)
    ADD_DEFINITIONS(-DOSPRAY_MPI=1)
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

ENDMACRO()


MACRO(CONFIGURE_OSPRAY)

  CONFIGURE_OSPRAY_NO_ARCH()

ENDMACRO()
