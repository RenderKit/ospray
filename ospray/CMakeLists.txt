
# configure "OSPRAY_LIB_SUFFIX" etc
CONFIGURE_OSPRAY()

IF (OSPRAY_MPI)
	# make sure that we compile with mpiicpc etc
	CONFIGURE_MPI()
ENDIF()

# build the ospray library
ADD_ISPC_LIBRARY(ospray${OSPRAY_LIB_SUFFIX} SHARED

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

	api/api.cpp
	api/device.cpp
	api/localdevice.cpp
	)
TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX} ${EMBREE_LIB})

IF (OSPRAY_MPI)
	ADD_LIBRARY(ospray_mpi${OSPRAY_LIB_SUFFIX} STATIC
		mpi/mpidevice.cpp
		mpi/mpicommon.cpp
		mpi/mpiloadbalancer.cpp
		mpi/worker.cpp
		mpi/handle.cpp
		)
	TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX} ospray_mpi${OSPRAY_LIB_SUFFIX})

	# add the worker stub
	ADD_EXECUTABLE(ospray_mpi_worker${OSPRAY_EXE_SUFFIX} mpi/mpiworker.cpp)
	TARGET_LINK_LIBRARIES(ospray_mpi_worker${OSPRAY_EXE_SUFFIX} ospray${OSPRAY_LIB_SUFFIX})
ENDIF()

#ADD_SUBDIRECTORY(api)








