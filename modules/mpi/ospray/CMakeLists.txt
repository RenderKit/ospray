## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
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

## Options ##

set(OSP_MPI_COMPRESSION_THRESHOLD 16 CACHE STRING
  "Min. number of ranks to enable message compression at (number)" )
mark_as_advanced(OSP_MPI_COMPRESSION_THRESHOLD)

## Build the MPI module ##

ispc_include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(ospray_module_mpi SHARED)

ispc_target_add_sources(ospray_module_mpi
  MPIOffloadDevice.cpp
  MPIOffloadWorker.cpp
  MPIDistributedDevice.cpp
  ModuleInit.cpp
  common/DistributedWorld.cpp
  common/DistributedWorld.ispc

  common/OSPWork.cpp
  common/Profiling.cpp
  common/Messaging.cpp

  fb/DistributedFrameBuffer.cpp
  fb/DistributedFrameBuffer.ispc
  fb/DistributedFrameBuffer_TileMessages.cpp
  fb/TileOperation.cpp

  render/DistributedLoadBalancer.cpp
  render/WriteMultipleTileOperation.cpp
  render/distributed/DistributedRaycast.cpp
  render/distributed/DistributedRaycast.ispc
  render/distributed/DistributedRenderer.cpp
  render/distributed/DistributedRenderer.ispc
  render/distributed/AlphaCompositeTileOperation.cpp
)

ospray_install_library(ospray_module_mpi COMPONENT mpi)

set_target_properties(ospray_module_mpi
  PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})

target_link_libraries(ospray_module_mpi
  PRIVATE
    snappy
  PUBLIC
    ospray_module_ispc
    ospray_mpi_common
    ospray_pico_bench
    ospcommon::ospcommon
)

target_include_directories(ospray_module_mpi
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(ospray_module_mpi PUBLIC
  -DOSP_MPI_COMPRESSION_THRESHOLD=${OSP_MPI_COMPRESSION_THRESHOLD})

## Build the worker executable ##

add_executable(ospray_mpi_worker mpi_offload_worker_main.cpp)
target_link_libraries(ospray_mpi_worker PRIVATE ospray_module_ispc)
if (WIN32)
  set_target_properties(ospray_mpi_worker
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
endif()

install(TARGETS ospray_mpi_worker
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT mpi
)

