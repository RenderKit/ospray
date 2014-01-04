CONFIGURE_OSPRAY()

IF (OSPRAY_MPI)
	ADD_DEFINITIONS(-DOSPRAY_MPI=1)
ENDIF()

ADD_LIBRARY(osp_api${OSPRAY_LIB_SUFFIX} SHARED
	api.cpp
	device.cpp
	localdevice.cpp
	)
TARGET_LINK_LIBRARIES(osp_api${OSPRAY_LIB_SUFFIX} osp_core${OSPRAY_LIB_SUFFIX})





##################################################################
# Code that is only included if MPI support is compiled in
##################################################################
IF (OSPRAY_MPI)
	CONFIGURE_MPI()
	ADD_LIBRARY(osp_mpi${OSPRAY_LIB_SUFFIX} STATIC
		mpidevice.cpp
		)
	TARGET_LINK_LIBRARIES(osp_api${OSPRAY_LIB_SUFFIX} osp_mpi${OSPRAY_LIB_SUFFIX})
ENDIF()





