CONFIGURE_OSPRAY()
CONFIGURE_MPI()

ADD_LIBRARY(ospray_mpi${OSPRAY_LIB_SUFFIX} STATIC
	mpidevice.cpp
	mpicommon.cpp
	mpiloadbalancer.cpp
	worker.cpp
	handle.cpp
	)
#SET (CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_INIT} -mt_)
TARGET_LINK_LIBRARIES(ospray_mpi${OSPRAY_LIB_SUFFIX}
#	mpi_mt
	${MPI_LIBRARIES}
	)

ADD_EXECUTABLE(ospray_mpi_worker${OSPRAY_EXE_SUFFIX}
	mpiworker.cpp
	)
TARGET_LINK_LIBRARIES(ospray_mpi_worker${OSPRAY_EXE_SUFFIX} 
	ospray${OSPRAY_LIB_SUFFIX}
	ospray_mpi${OSPRAY_LIB_SUFFIX}
	embree
	${MPI_LIBRARIES}
	)

