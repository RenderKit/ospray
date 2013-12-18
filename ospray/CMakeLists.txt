##################################################################
# Output path configuration
##################################################################
CONFIGURE_OSPRAY()

##################################################################
# build embree
##################################################################
INCLUDE(${EMBREE_DIR}/common/cmake/ispc.cmake)
INCLUDE_DIRECTORIES(${EMBREE_DIR})
INCLUDE_DIRECTORIES(${EMBREE_DIR}/common)
INCLUDE_DIRECTORIES(${EMBREE_DIR}/kernels)
INCLUDE_DIRECTORIES(${EMBREE_DIR}/include)
INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR} ${PROJECT_SOURCE_DIR})
IF (THIS_IS_MIC) #(OSPRAY_TARGET STREQUAL "MIC")
	SET(COMPILER "ICC")
#	SET(TARGET_SSE41 ON)
	INCLUDE(${EMBREE_DIR}/common/cmake/icc-xeonphi.cmake)
ELSE()
	SET(TARGET_SSE41 ON)
	SET(TARGET_AVX ON)
	SET(TARGET_AVX2 ON)
	INCLUDE(${EMBREE_DIR}/common/cmake/icc.cmake)
	INCLUDE(${EMBREE_DIR}/common/cmake/ispc.cmake)
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

ISPC_COMPILE(${OSPRAY_TARGET} render/testrenderer.ispc)

ADD_LIBRARY(OSPcore 
	fb/swapchain.cpp 
	fb/framebuffer.cpp
	common/model.cpp
	render/testrenderer.cpp
	render/testrenderer.dev${OSPRAY_ISPC_SUFFIX}
	)
TARGET_LINK_LIBRARIES(OSPcore OSPcommon)

ADD_SUBDIRECTORY(api)

