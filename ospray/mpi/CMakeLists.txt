## ======================================================================== ##
## Copyright 2009-2016 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

# if mpi mode is enabled, we have to configure the right mpi
# compiler etc.
OSPRAY_CONFIGURE_MPI()

OSPRAY_CREATE_LIBRARY(ospray_mpi
  buffers.h
  command.h

  MPIDevice.cpp
  MPICommon.cpp
  MPILoadBalancer.cpp
  worker.cpp

  async/Messaging.cpp
  async/SimpleSendRecvMessaging.cpp
  async/MultiIsendIrecvMessaging.cpp
  async/BatchedIsendIrecvMessaging.cpp
  async/CommLayer.cpp

  DistributedFrameBuffer.cpp
  DistributedFrameBuffer.ispc
  DistributedFrameBuffer_TileTypes.cpp

LINK
  ospray
  ospray_common
  ${MPI_CXX_LIBRARIES}
)

##############################################################
# MPI DEVICE - mpi worker
##############################################################

OSPRAY_CREATE_APPLICATION(ospray_mpi_worker
  MPIWorker.cpp
LINK
  ospray
  ospray_common
  ospray_mpi
)
