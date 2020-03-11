## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## Options ##

set(OSP_MPI_COMPRESSION_THRESHOLD 16 CACHE STRING
  "Min. number of ranks to enable message compression at (number)" )
mark_as_advanced(OSP_MPI_COMPRESSION_THRESHOLD)

## Build the MPI module ##

ispc_include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(ospray_module_mpi SHARED)

ispc_target_add_sources(ospray_module_mpi
  ${OSPRAY_MPI_RESOURCE}

  MPIOffloadDevice.cpp
  MPIOffloadWorker.cpp
  MPIDistributedDevice.cpp
  MPIDistributedDevice.ispc
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

target_link_libraries(ospray_module_mpi
  PRIVATE
    snappy
    ospray_mpi_common
    ospray_pico_bench
    ospray::ospray_sdk
    ospray::ospray_module_ispc
)

target_include_directories(ospray_module_mpi
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(ospray_module_mpi PUBLIC
  -DOSP_MPI_COMPRESSION_THRESHOLD=${OSP_MPI_COMPRESSION_THRESHOLD})

set_target_properties(ospray_module_mpi
  PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

install(TARGETS ospray_module_mpi
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_SKIP
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(TARGETS ospray_module_mpi
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_ONLY
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

## Build the worker executable ##

add_executable(ospray_mpi_worker
  ${OSPRAY_MPI_RESOURCE} mpi_offload_worker_main.cpp)

target_link_libraries(ospray_mpi_worker PRIVATE ospray::ospray)

install(TARGETS ospray_mpi_worker DESTINATION ${CMAKE_INSTALL_BINDIR})
