
# configure this dir for the proper "OSPRAY_LIB_SUFFIX" etc (depending
# on which target we build for )
CONFIGURE_OSPRAY()

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

IF (OSPRAY_MPI)
	# if mpi mode is enabled, we have to configure the right mpi
	# compiler etc.
	CONFIGURE_MPI()

	# files to add to libospray - add these *only* if we actually build
	# for MPI
	SET(OSPRAY_MPI_COMPONENTS 
		mpi/mpidevice.cpp
		mpi/mpicommon.cpp
		mpi/mpiloadbalancer.cpp
		mpi/worker.cpp
		mpi/handle.cpp
		)
ENDIF()

# build the ospray library (using flags and compilers as configured by
# CONFIGURE_OSPRAY() and/or CONFIGURE_MPI()
OSPRAY_ADD_LIBRARY(ospray${OSPRAY_LIB_SUFFIX} SHARED
	common/ospcommon.cpp
	common/managed.cpp
	common/data.cpp
	common/model.cpp
	common/library.cpp

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
	volume/bricked32.cpp
	volume/naive32.cpp

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

	${OSPRAY_MPI_COMPONENTS}
	)
TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX} ${EMBREE_LIB})

# add a dedicated MPI worker that ospray can launch onto any worker
# node (including MIC nodes)
IF (OSPRAY_MPI)
	ADD_EXECUTABLE(ospray_mpi_worker${OSPRAY_EXE_SUFFIX} mpi/mpiworker.cpp)
	TARGET_LINK_LIBRARIES(ospray_mpi_worker${OSPRAY_EXE_SUFFIX} ospray${OSPRAY_LIB_SUFFIX})
ENDIF()








