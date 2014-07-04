# ##################################################################
# this module builds the embree libs (libembree...so) and sets
# a variable "OSPRAY_EMBREE_CXX_FLAGS" that is to be passed as a compiler
# flag to every ospray file that includes embree files
# ##################################################################

SET(OSPRAY_EMBREE_CXX_FLAGS "")

##################################################################
# where embree can be found
##################################################################
SET(EMBREE_DIR ${PROJECT_SOURCE_DIR}/../embree CACHE STRING
	"Embree 2.1 directory")
MARK_AS_ADVANCED(EMBREE_DIR)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${EMBREE_DIR}/common/cmake)

##################################################################
# build embree - global settings independent of target
##################################################################

# make sure embree doesn't hide its symbols
SET(RTCORE_EXPORT_ALL_SYMBOLS ON)
ADD_DEFINITIONS(-D__EXPORT_ALL_SYMBOLS__)

# just to be sure, make sure embree fixes rays
SET(RTCORE_FIX_RAYS ON)
ADD_DEFINITIONS(-D__FIX_RAYS__)


# this has to be called *AFTER* setting RTCORE_EXPORT_ALL_SYMBOLS
IF(OSPRAY_ICC)
	INCLUDE(${EMBREE_DIR}/common/cmake/icc.cmake)
ELSE()
	INCLUDE(${EMBREE_DIR}/common/cmake/gcc.cmake)
ENDIF()


##################################################################
# build embree - target specific settings
##################################################################
IF (${OSPRAY_XEON_TARGET} STREQUAL "AVX2")
	# tell embree to build the avx2 kernels
	SET(TARGET_SSE41  ON)
	SET(TARGET_AVX  ON)
	SET(TARGET_AVX2  ON)
	# tell embree to build avx2 ispc binding
  SET(ISPC_TARGETS "avx2")
ELSEIF (${OSPRAY_XEON_TARGET} STREQUAL "AVX")
	# tell embree to build the avx(1) kernels
	SET(TARGET_SSE41  ON)
	SET(TARGET_AVX  ON)
	# tell embree to build avx(1) ispc binding
  SET(ISPC_TARGETS "avx")
ELSEIF (${OSPRAY_XEON_TARGET} STREQUAL "SSE")
	# tell embree to build the avx(1) kernels
	SET(TARGET_SSE41  ON)
	# tell embree to build avx(1) ispc binding
  SET(ISPC_TARGETS "sse4")
ELSE()
	message("build_emrbre.cmake: target '${OSPRAY_XEON_TARGET}' NOT CONFIGURED...")
ENDIF()

IF (OSPRAY_MIC) 
	SET(TARGET_XEON_PHI ON)
ENDIF()

INCLUDE(${EMBREE_DIR}/common/cmake/ispc.cmake)

# include paths requried for including embree headers
INCLUDE_DIRECTORIES(${EMBREE_DIR})
INCLUDE_DIRECTORIES(${EMBREE_DIR}/common)
INCLUDE_DIRECTORIES(${EMBREE_DIR}/kernels)
INCLUDE_DIRECTORIES(${EMBREE_DIR}/include)

# include paths requried for including embree ISPC headers
INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
                
ADD_SUBDIRECTORY(${EMBREE_DIR}/common  embree_common)
ADD_SUBDIRECTORY(${EMBREE_DIR}/kernels embree_kernels)





