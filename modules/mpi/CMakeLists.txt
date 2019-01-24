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

option(OSPRAY_MODULE_MPI "MPI parallel device" OFF)

if (OSPRAY_MODULE_MPI)
  option(OSPRAY_MODULE_MPI_APPS "MPI module viewer application"
    ${OSPRAY_APPS_EXAMPLEVIEWER})

  set(OSP_MPI_COMPRESSION_THRESHOLD 16 CACHE STRING
    "Min. number of ranks to enable message compression at (number)" )
  mark_as_advanced(OSP_MPI_COMPRESSION_THRESHOLD)

  set(OSPRAY_DEFAULT_COMPONENT mpi)

  ospray_build_component(mpiCommon)
  ospray_build_component(mpiMessageLayer)
  ospray_build_component(pico_bench)
  ospray_build_component(snappy EXCLUDE_FROM_ALL)

  include_directories_ispc(${CMAKE_CURRENT_BINARY_DIR})

  ospray_create_library(ospray_module_mpi
    MPIDistributedDevice.cpp
    MPIOffloadDevice.cpp
    MPIOffloadWorker.cpp

    common/OSPWork.cpp
    common/Messaging.cpp
    common/DistributedModel.cpp
    common/DistributedModel.ispc
    common/Profiling.cpp

    fb/DistributedFrameBuffer.cpp
    fb/DistributedFrameBuffer.ispc
    fb/DistributedFrameBuffer_TileTypes.cpp
    fb/DistributedFrameBuffer_TileMessages.cpp

    fb/DebugPixelOp.cpp

    render/MPILoadBalancer.cpp
    render/distributed/DistributedRaycast.cpp
    render/distributed/DistributedRaycast.ispc
  )

  target_link_libraries(ospray_module_mpi
    PRIVATE
      snappy
    PUBLIC
      ospray_module_ispc
      ospray_mpi_common
      ospray_mpi_maml
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

  ##############################################################
  # MPI DEVICE - mpi worker
  ##############################################################

  ospray_create_application(ospray_mpi_worker mpi_offload_worker_main.cpp)
  target_link_libraries(ospray_mpi_worker PRIVATE ospray_module_ispc)

  ##############################################################
  # Test apps
  ##############################################################
  if (OSPRAY_MODULE_MPI_APPS)
    # Note: the MPI example viewer depends on tfn lib, so we must make sure that
    # was built.
    if (NOT TARGET ospray_tfn OR NOT TARGET ospray_imgui_util_sg
        OR NOT TARGET ospray_imgui3d_sg)
      message(FATAL_ERROR "ospray_tfn library is required for the MPI example app,"
        " please set -DOSPRAY_APPS_EXAMPLEVIEWER=ON as well")
    endif ()

    add_subdirectory(apps)
  endif (OSPRAY_MODULE_MPI_APPS)

endif (OSPRAY_MODULE_MPI)
