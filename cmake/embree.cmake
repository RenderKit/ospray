##################################################################
# where embree can be found
##################################################################
SET(EMBREE_DIR ${PROJECT_SOURCE_DIR}/../embree CACHE STRING
	"Embree 2.1 directory")
SET(CMAKE_MODULE_PATH ${EMBREE_DIR}/common/cmake ${CMAKE_MODULE_PATH})
MARK_AS_ADVANCED(EMBREE_DIR)

##################################################################
# build embree
##################################################################
INCLUDE(${EMBREE_DIR}/common/cmake/ispc.cmake)
INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

INCLUDE_DIRECTORIES(${EMBREE_DIR})
INCLUDE_DIRECTORIES(${EMBREE_DIR}/common)
INCLUDE_DIRECTORIES(${EMBREE_DIR}/kernels)
INCLUDE_DIRECTORIES(${EMBREE_DIR}/include)
INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR} ${EMBREE_DIR}/tutorials ${PROJECT_SOURCE_DIR})

MARK_AS_ADVANCED(USE_STAT_COUNTERS)

UNSET(TARGET_SSE41 CACHE)
UNSET(TARGET_AVX   CACHE)
UNSET(TARGET_AVX2  CACHE)

SET(RTCORE_EXPORT_ALL_SYMBOLS ON)

IF (${OSPRAY_XEON_TARGET} STREQUAL "AVX2")
#	SET(TARGET_SSE41 OFF)
#	SET(TARGET_AVX   OFF)
	SET(TARGET_AVX   ON)
	SET(TARGET_AVX2  ON)
	SET(ISPC_TARGETS "avx2")
#	message("set AVX2")
ELSEIF (${OSPRAY_XEON_TARGET} STREQUAL "AVX")
#	SET(TARGET_SSE41 OFF)
	SET(TARGET_AVX   ON)
	SET(ISPC_TARGETS "avx")
#	SET(TARGET_AVX2  OFF)
#	message("set AVX")
ELSEIF (${OSPRAY_XEON_TARGET} STREQUAL "SSE")
	SET(TARGET_SSE41 ON)
	SET(ISPC_TARGETS "sse41")
#	SET(TARGET_AVX   OFF)
#	SET(TARGET_AVX2  OFF)
#	message("set SSE41")
ELSE()
	MESSAGE("unknown OSPRAY_XEON_TARGET '${OSPRAY_XEON_TARGET}'")
ENDIF()

IF (OSPRAY_MIC) 
	SET(TARGET_XEON_PHI ON)
ENDIF()

IF (OSPRAY_ICC)
	INCLUDE(${EMBREE_DIR}/common/cmake/icc.cmake)
ELSE()
	INCLUDE(${EMBREE_DIR}/common/cmake/gcc.cmake)
ENDIF()
SET(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
#SET(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")

ADD_SUBDIRECTORY(${EMBREE_DIR}/common  embree_common)
ADD_SUBDIRECTORY(${EMBREE_DIR}/kernels embree_kernels)

#######################################################
# and to use embree
#######################################################
INCLUDE_DIRECTORIES(${EMBREE_DIR}/include/embree2)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/lib)

