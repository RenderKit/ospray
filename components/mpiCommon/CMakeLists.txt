

OSPRAY_CONFIGURE_MPI()

OSPRAY_CREATE_LIBRARY(ospray_mpi_common

  MPICommon.cpp
  async/CommLayer.cpp
  async/Messaging.cpp
  async/BatchedIsendIrecvMessaging.cpp
  
  LINK

  ${MPI_LIBRARIES}
  ospray_common
  )