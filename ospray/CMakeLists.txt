
# configure "OSPRAY_LIB_SUFFIX" etc
CONFIGURE_OSPRAY()

ADD_ISPC_LIBRARY(ospray_core${OSPRAY_LIB_SUFFIX} STATIC   

	common/ospcommon.cpp
	common/managed.cpp
	common/data.cpp
	common/model.cpp

	fb/swapchain.cpp 
	fb/framebuffer.cpp
	fb/localfb.ispc

	camera/camera.cpp
	camera/perspectivecamera.ispc
	camera/perspectivecamera.cpp

	volume/volume.ispc
	volume/bricked32.ispc
	volume/naive32.ispc
	volume/volume.cpp

	geometry/geometry.cpp
	geometry/trianglemesh.cpp

	render/renderer.cpp
	render/testrenderer.ispc
	render/raycast.ispc
	render/dvr.ispc
	render/tilerenderer.cpp
	render/loadbalancer.cpp
	render/testrenderer.cpp
	render/raycast.cpp
	render/dvr.cpp
	)
TARGET_LINK_LIBRARIES(ospray_core${OSPRAY_LIB_SUFFIX} ${EMBREE_LIB})


ADD_LIBRARY(ospray${OSPRAY_LIB_SUFFIX} SHARED
	api/api.cpp
	api/device.cpp
	api/localdevice.cpp
	)


IF (OSPRAY_MPI)
	ADD_SUBDIRECTORY(mpi)
ENDIF()


IF (OSPRAY_MPI)
	TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX} ospray_mpi${OSPRAY_LIB_SUFFIX})
ELSE()
	TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX} ospray_core${OSPRAY_LIB_SUFFIX})
ENDIF()

#ADD_SUBDIRECTORY(api)








