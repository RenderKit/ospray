## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
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
IF (OSPRAY_MODULE_MPI)
  # if mpi mode is enabled, we have to configure the right mpi
  # compiler etc.
  OSPRAY_CONFIGURE_MPI()

  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)

  OSPRAY_CREATE_LIBRARY(ospray_module_mpi
    MPIDevice.cpp
    worker.cpp

    common/buffers.h
    common/command.h
    common/CommandStream.h
    common/Work.cpp
    common/Core.cpp
    common/MPICommon.cpp
    common/BufferedMPIComm.cpp
    common/command.cpp
    common/async/Messaging.cpp
    common/async/SimpleSendRecvMessaging.cpp
    common/async/MultiIsendIrecvMessaging.cpp
    common/async/BatchedIsendIrecvMessaging.cpp
    common/async/CommLayer.cpp

    fb/DistributedFrameBuffer.cpp
    fb/DistributedFrameBuffer.ispc
    fb/DistributedFrameBuffer_TileTypes.cpp

    render/MPILoadBalancer.cpp
    render/volume/RaycastVolumeMaterial.cpp
    render/volume/RaycastVolumeMaterial.ih
    render/volume/RaycastVolumeMaterial.ispc
    render/volume/RaycastVolumeRenderer.cpp
    render/volume/RaycastVolumeRenderer.ispc

    volume/DataDistributedBlockedVolume.ispc
    volume/DataDistributedBlockedVolume.cpp

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
    ospray_module_mpi
  )

ENDIF (OSPRAY_MODULE_MPI)
