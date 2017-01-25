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

OPTION(OSPRAY_MODULE_MPI "MPI parallel device" OFF)

IF (OSPRAY_MODULE_MPI)

  OSPRAY_BUILD_COMPONENT(mpiCommon)
  
  OPTION(OSPRAY_EXP_DATA_PARALLEL "Experimental data-parallel compositing mode" ON)
  OPTION(OSPRAY_PIN_ASYNC "Pin async mpi comm threads?" OFF)
  MARK_AS_ADVANCED(OSPRAY_PIN_ASYNC)
  
  # if mpi mode is enabled, we have to configure the right mpi
  # compiler etc.
  OSPRAY_CONFIGURE_MPI()

  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)

  OSPRAY_CREATE_LIBRARY(ospray_module_mpi
    MPIDevice.cpp
    worker.cpp

    common/OSPWork.cpp
    common/BufferedMPIComm.cpp
    common/BufferedDataStreaming.cpp

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
    ospray_mpi_common
    ospray_common
    ${MPI_CXX_LIBRARIES}
  )

  ##############################################################
  # MPI DEVICE - mpi worker
  ##############################################################

  OSPRAY_CREATE_APPLICATION(ospray_mpi_worker
    MPIWorker.cpp
  LINK
#    ospray
#    ospray_common
    ospray_module_mpi
#    ${MPI_CXX_LIBRARIES}
  )

ENDIF (OSPRAY_MODULE_MPI)
