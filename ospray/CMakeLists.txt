##################################################################
# Output path configuration
##################################################################
CONFIGURE_OSPRAY()
CMAKE_POLICY(SET CMP0002 OLD)

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
	render/raycast.ispc
	fb/localfb.ispc
	)

ADD_LIBRARY(osp_core${OSPRAY_LIB_SUFFIX} SHARED
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
TARGET_LINK_LIBRARIES(osp_core${OSPRAY_LIB_SUFFIX} osp_common${OSPRAY_LIB_SUFFIX})

ADD_SUBDIRECTORY(api)

