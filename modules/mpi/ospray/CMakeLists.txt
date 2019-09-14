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

ospray_create_library(ospray_module_mpi
  MPIOffloadDevice.cpp
  MPIOffloadWorker.cpp

  common/OSPWork.cpp
  common/Messaging.cpp
  common/Profiling.cpp

  fb/DistributedFrameBuffer.cpp
  fb/DistributedFrameBuffer.ispc
  fb/DistributedFrameBuffer_TileTypes.cpp
  fb/DistributedFrameBuffer_TileMessages.cpp

  render/MPILoadBalancer.cpp
)

target_link_libraries(ospray_module_mpi
  PRIVATE
    snappy
  PUBLIC
    ospray_module_ispc
    ospray_mpi_common
    ospray_pico_bench
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

ospray_create_application(ospray_mpi_worker mpi_offload_worker_main.cpp)
target_link_libraries(ospray_mpi_worker PRIVATE ospray_module_ispc)
