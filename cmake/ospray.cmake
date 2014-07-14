FILE(WRITE "${CMAKE_BINARY_DIR}/CMakeDefines.h" "#define CMAKE_BUILD_DIR \"${CMAKE_BINARY_DIR}\"\n")

#include bindir - that's where ispc puts generated header files
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
SET(OSPRAY_BINARY_DIR ${CMAKE_BINARY_DIR})
SET(OSPRAY_DIR ${PROJECT_SOURCE_DIR})
# arch-specific cmd-line flags for various arch and compiler configs

# Configure the output directories. To allow IMPI to do its magic we
# will put *exexecutables* into the (same) build directory, but tag
# mic-executables with ".mic". *libraries* cannot use the
# ".mic"-suffix trick, so we'll put libraries into separate
# directories (names 'intel64' and 'mic', respectively)
MACRO(CONFIGURE_OSPRAY)
  SET(LIBRARY_OUTPUT_PATH ${OSPRAY_BINARY_DIR})
  SET(EXECUTABLE_OUTPUT_PATH ${OSPRAY_BINARY_DIR})
  LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})

  IF (OSPRAY_TARGET STREQUAL "MIC")
    SET(OSPRAY_EXE_SUFFIX ".mic")
    SET(OSPRAY_LIB_SUFFIX "_mic")
    SET(OSPRAY_ISPC_SUFFIX ".cpp")
    SET(OSPRAY_ISPC_TARGET "mic")
    SET(THIS_IS_MIC ON)
    SET(__XEON__ OFF)
    #		SET(LIBRARY_OUTPUT_PATH "${OSPRAY_BINARY_DIR}/lib/mic")
    ADD_DEFINITIONS(-DOSPRAY_SPMD_WIDTH=16)
    ADD_DEFINITIONS(-DOSPRAY_TARGET_MIC=1)
  ELSE()
    SET(OSPRAY_EXE_SUFFIX "")
    SET(OSPRAY_LIB_SUFFIX "")
    SET(OSPRAY_ISPC_SUFFIX ".o")
    SET(THIS_IS_MIC OFF)
    SET(__XEON__ ON)
    IF (OSPRAY_ICC)
      INCLUDE(${PROJECT_SOURCE_DIR}/cmake/icc.cmake)
    ELSE()
      INCLUDE(${PROJECT_SOURCE_DIR}/cmake/gcc.cmake)
    ENDIF()

    IF (${OSPRAY_XEON_TARGET} STREQUAL "AVX2")
      ADD_DEFINITIONS(-DOSPRAY_SPMD_WIDTH=8)
      ADD_DEFINITIONS(-DOSPRAY_TARGET_AVX2=1)
      SET(OSPRAY_ISPC_TARGET "avx2")
    ELSEIF (${OSPRAY_XEON_TARGET} STREQUAL "AVX")
      ADD_DEFINITIONS(-DOSPRAY_SPMD_WIDTH=8)
      ADD_DEFINITIONS(-DOSPRAY_TARGET_AVX=1)
      SET(OSPRAY_ISPC_TARGET "avx")
    ELSEIF (${OSPRAY_XEON_TARGET} STREQUAL "SSE")
      ADD_DEFINITIONS(-DOSPRAY_SPMD_WIDTH=4)
      ADD_DEFINITIONS(-DOSPRAY_TARGET_SSE=1)
      SET(OSPRAY_ISPC_TARGET "sse4")
    ELSE()
      MESSAGE("unknown OSPRAY_XEON_TARGET '${OSPRAY_XEON_TARGET}'")
    ENDIF()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OSPRAY_ARCH_FLAGS__${OSPRAY_XEON_TARGET}}")
  ENDIF()
  
  IF (OSPRAY_MPI)
    ADD_DEFINITIONS(-DOSPRAY_MPI=1)
  ENDIF()

  IF (THIS_IS_MIC)
    INCLUDE(${EMBREE_DIR}/common/cmake/icc_xeonphi.cmake)
    
    SET(EMBREE_LIB embree_xeonphi)

    # whehter to build in MIC/xeon phi support
    SET(OSPRAY_BUILD_COI_DEVICE OFF CACHE BOOL "Build COI Device for OSPRay's MIC support?")

  ELSE()
    SET(EMBREE_LIB embree)
  ENDIF()

  INCLUDE(ospray_ispc)
ENDMACRO()


