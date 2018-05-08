## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
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

OPTION(OSPRAY_MODULE_MPI "MPI parallel device" OFF)

IF (OSPRAY_MODULE_MPI)

  SET(OSPRAY_DEFAULT_COMPONENT mpi)

  OSPRAY_CONFIGURE_MPI()

  OSPRAY_BUILD_COMPONENT(mpiCommon)
  OSPRAY_BUILD_COMPONENT(mpiMessageLayer)

  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
  INCLUDE_DIRECTORIES_ISPC(${CMAKE_CURRENT_BINARY_DIR})

  OSPRAY_CREATE_LIBRARY(ospray_module_mpi
    MPIDistributedDevice.cpp
    MPIOffloadDevice.cpp
    MPIOffloadWorker.cpp

    common/OSPWork.cpp
    common/Messaging.cpp
    common/DistributedModel.cpp
    common/DistributedModel.ispc

    fb/DistributedFrameBuffer.cpp
    fb/DistributedFrameBuffer.ispc
    fb/DistributedFrameBuffer_TileTypes.cpp

    render/MPILoadBalancer.cpp
    render/distributed/DistributedRaycast.cpp
    render/distributed/DistributedRaycast.ispc

  LINK

    ospray_module_ispc
    ospray_mpi_common
    ospray_mpi_maml
  )

  ##############################################################
  # MPI DEVICE - mpi worker
  ##############################################################

  OSPRAY_CREATE_APPLICATION(ospray_mpi_worker
    mpi_offload_worker_main.cpp
  LINK
    ospray_module_ispc
  )

  ##############################################################
  # Test apps
  ##############################################################

  ADD_SUBDIRECTORY(apps)

ENDIF (OSPRAY_MODULE_MPI)
