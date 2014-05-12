
# configure this dir for the proper "OSPRAY_LIB_SUFFIX" etc (depending
# on which target we build for )
CONFIGURE_OSPRAY()

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
#INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
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

IF (OSPRAY_MIC_COI)
	ADD_DEFINITIONS(-DOSPRAY_MIC_COI=1)
	IF (THIS_IS_MIC)
		ADD_DEFINITIONS(-DOSPRAY_COI_DEV=1)
		ADD_DEFINITIONS(-DOSPRAY_COI_HOST=0)
		SET(OSPRAY_COI_COMPONENTS api/coidevice_dev.cpp)
	ELSE()
		ADD_DEFINITIONS(-DOSPRAY_COI_DEV=0)
		ADD_DEFINITIONS(-DOSPRAY_COI_HOST=1)
		SET(OSPRAY_COI_COMPONENTS api/coidevice_host.cpp)
	ENDIF()
ELSE()
	SET(OSPRAY_COI_COMPONENTS "")
ENDIF()

# build the ospray library (using flags and compilers as configured by
# CONFIGURE_OSPRAY() and/or CONFIGURE_MPI()
OSPRAY_ADD_LIBRARY(ospray${OSPRAY_LIB_SUFFIX} SHARED
	common/ospcommon.cpp
	common/managed.cpp
	common/data.cpp
	common/model.cpp
	common/model.ispc
	common/material.cpp
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

	geometry/geometry.ispc
	geometry/geometry.cpp
	geometry/trianglemesh.ispc
	geometry/trianglemesh.cpp
	geometry/streamlines.cpp
	geometry/streamlines.ispc
	geometry/instance.ispc
	geometry/instance.cpp
	geometry/spheres.cpp
	geometry/spheres.ispc
	geometry/cylinders.cpp
	geometry/cylinders.ispc

	render/tilerenderer.cpp
	render/loadbalancer.cpp

	render/renderer.cpp
	render/raycast/raycast.cpp
	render/raycast/raycast.ispc
	render/ao16/ao16.cpp
	render/ao16/ao16.ispc
	render/dvr/dvr.cpp
	render/dvr/dvr.ispc
	render/obj/objrenderer.ispc
	render/obj/objmaterial.ispc
	render/obj/objrenderer.cpp
	render/obj/objmaterial.cpp

	api/api.cpp
	api/device.cpp
	api/localdevice.cpp

	${OSPRAY_MPI_COMPONENTS}

	${OSPRAY_COI_COMPONENTS}
	)
TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX} ${EMBREE_LIB})

# add a dedicated MPI worker that ospray can launch onto any worker
# node (including MIC nodes)
IF (OSPRAY_MPI)
	ADD_EXECUTABLE(ospray_mpi_worker${OSPRAY_EXE_SUFFIX} mpi/mpiworker.cpp)
	TARGET_LINK_LIBRARIES(ospray_mpi_worker${OSPRAY_EXE_SUFFIX} ospray${OSPRAY_LIB_SUFFIX})
ENDIF()








