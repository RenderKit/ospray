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



IF (ISPC_PLUS)
	SET(ISPC_BACKEND /home/wald/Projects/ISPC++/backend/${ISPC_PLUS_BACKEND}/${ISPC_PLUS_BACKEND}.cpp)
ELSE()
	SET(ISPC_BACKEND "")
ENDIF()


#ADD_SUBDIRECTORY(common)

ISPC_COMPILE(${OSPRAY_ISPC_TARGET} 
	render/testrenderer.ispc
	render/raycast.ispc
	render/dvr.ispc
	fb/localfb.ispc
	camera/perspectivecamera.ispc
	volume/volume.ispc
	volume/bricked32.ispc
	volume/naive32.ispc
	)

ADD_LIBRARY(osp_core${OSPRAY_LIB_SUFFIX} SHARED
	common/ospcommon.cpp
	common/managed.cpp
	common/data.cpp
	fb/swapchain.cpp 
	fb/framebuffer.cpp
	common/model.cpp
	geometry/geometry.cpp
	geometry/trianglemesh.cpp
	render/renderer.cpp
	render/tilerenderer.cpp
	render/loadbalancer.cpp
	render/testrenderer.cpp
	render/testrenderer.dev${OSPRAY_ISPC_SUFFIX}
	render/raycast.cpp
	render/raycast.dev${OSPRAY_ISPC_SUFFIX}
	render/dvr.cpp
	render/dvr.dev${OSPRAY_ISPC_SUFFIX}
	volume/volume.cpp
	volume/volume.dev${OSPRAY_ISPC_SUFFIX}
	volume/naive32.dev${OSPRAY_ISPC_SUFFIX}
	volume/bricked32.dev${OSPRAY_ISPC_SUFFIX}
	camera/camera.cpp
	camera/perspectivecamera.cpp
	camera/perspectivecamera.dev${OSPRAY_ISPC_SUFFIX}
	fb/localfb.dev${OSPRAY_ISPC_SUFFIX}

	${ISPC_BACKEND}
	)
IF (ISPC_PLUS)
	SET_TARGET_PROPERTIES(osp_core${OSPRAY_LIB_SUFFIX} PROPERTIES COMPILE_FLAGS "${FLAGS_AVX2}")
ENDIF()
#TARGET_LINK_LIBRARIES(osp_core${OSPRAY_LIB_SUFFIX} osp_common${OSPRAY_LIB_SUFFIX})
TARGET_LINK_LIBRARIES(osp_core${OSPRAY_LIB_SUFFIX} ${EMBREE_LIB})

IF (OSPRAY_MPI)
	ADD_SUBDIRECTORY(mpi)
ENDIF()



ADD_SUBDIRECTORY(api)








