##################################################################
# Output path configuration
##################################################################
CONFIGURE_OSPRAY()

##################################################################
# build embree
##################################################################
#INCLUDE(${EMBREE_DIR}/common/cmake/ispc.cmake)
INCLUDE_DIRECTORIES(${EMBREE_DIR})
INCLUDE_DIRECTORIES(${EMBREE_DIR}/common)
INCLUDE_DIRECTORIES(${EMBREE_DIR}/kernels)
INCLUDE_DIRECTORIES(${EMBREE_DIR}/include)
INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR} ${EMBREE_DIR}/tutorials ${PROJECT_SOURCE_DIR})


IF (THIS_IS_MIC) #(OSPRAY_TARGET STREQUAL "MIC")
	SET(COMPILER "ICC")
#	SET(TARGET_SSE41 ON)
	INCLUDE(${EMBREE_DIR}/common/cmake/icc-xeonphi.cmake)
ELSE()
	IF (${OSPRAY_XEON_TARGET} STREQUAL "AVX2")
		SET(TARGET_AVX2 ON)
	ELSEIF (${OSPRAY_XEON_TARGET} STREQUAL "AVX")
		SET(TARGET_AVX ON)
	ELSEIF (${OSPRAY_XEON_TARGET} STREQUAL "SSE")
		SET(TARGET_SSE41 ON)
	ELSE()
		MESSAGE("unknown OSPRAY_XEON_TARGET '${OSPRAY_XEON_TARGET}'")
	ENDIF()
	IF (OSPRAY_ICC)
		INCLUDE(${EMBREE_DIR}/common/cmake/icc.cmake)
	ELSE()
		INCLUDE(${EMBREE_DIR}/common/cmake/gcc.cmake)
	ENDIF()
ENDIF()
ADD_SUBDIRECTORY(${EMBREE_DIR}/common  embree_common)
ADD_SUBDIRECTORY(${EMBREE_DIR}/kernels embree_kernels)

##################################################################
# the actual pieces that build ospray
##################################################################
#ADD_SUBDIRECTORY(common)

#ADD_SUBDIRECTORY(mpi)

# implements the API (in its different realization: local, service,
# and collaborative)

#ADD_LIBRARY(OSPrayCore
#	common/ospray.cpp
#	fb/framebuffer.cpp
#	fb/swapchain.cpp
#	)
#TARGET_LINK_LIBRARIES(OSPrayCore embree)

ADD_SUBDIRECTORY(common)

ISPC_COMPILE(${OSPRAY_TARGET} 
	render/testrenderer.ispc
	fb/localfb.ispc
	)

ADD_LIBRARY(OSPcore SHARED
	fb/swapchain.cpp 
	fb/framebuffer.cpp
	common/model.cpp
	geometry/geometry.cpp
	geometry/trianglemesh.cpp
	render/renderer.cpp
	render/testrenderer.cpp
	render/testrenderer.dev${OSPRAY_ISPC_SUFFIX}
	fb/localfb.dev${OSPRAY_ISPC_SUFFIX}
	)
TARGET_LINK_LIBRARIES(OSPcore OSPcommon)

ADD_SUBDIRECTORY(api)

