## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## Options ##

set(OSPRAY_MPI_COMPRESSION_THRESHOLD 16 CACHE STRING
  "Min. number of ranks to enable message compression at (number)" )
mark_as_advanced(OSPRAY_MPI_COMPRESSION_THRESHOLD)

## Build the MPI module ##

ispc_include_directories(
  ${PROJECT_SOURCE_DIR}/ospray/include
  ${PROJECT_SOURCE_DIR}/ospray
  ${PROJECT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${RKCOMMON_INCLUDE_DIRS}
  ${EMBREE_INCLUDE_DIRS}
)

add_library(ospray_module_mpi SHARED)

ispc_target_add_sources(ospray_module_mpi
  ${OSPRAY_RESOURCE}

  MPIOffloadDevice.cpp
  MPIOffloadWorker.cpp
  MPIDistributedDevice.cpp
  MPIDistributedDevice.ispc
  ModuleInit.cpp
  common/DistributedWorld.cpp

  common/OSPWork.cpp
  common/Messaging.cpp
  common/DynamicLoadBalancer.cpp

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
    Snappy::snappy
    rkcommon::rkcommon
    ospray_mpi_common
    ospray_pico_bench
    ospray_module_cpu
)

target_include_directories(ospray_module_mpi
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    # For ISPC module generated ispc headers
    ${PROJECT_BINARY_DIR}/ospray/
)

target_compile_definitions(ospray_module_mpi PUBLIC
  -DOSP_MPI_COMPRESSION_THRESHOLD=${OSPRAY_MPI_COMPRESSION_THRESHOLD})

ospray_install_library(ospray_module_mpi mpi)

## Build the worker executable ##

add_executable(ospray_mpi_worker
  ${OSPRAY_RESOURCE} mpi_offload_worker_main.cpp)

target_link_libraries(ospray_mpi_worker PRIVATE ospray)

install(TARGETS ospray_mpi_worker
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT mpi)

ospray_sign_target(ospray_mpi_worker)

# -------------------------------------------------------
# Install SDK headers
# -------------------------------------------------------

get_subdirectories(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

foreach(dir ${SUBDIRS})
  install(DIRECTORY ${dir}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK/modules/mpi
    COMPONENT devel
    FILES_MATCHING
    PATTERN *.h
    PATTERN *.ih
  )
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${dir}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK/modules/mpi
    COMPONENT devel
    FILES_MATCHING
    PATTERN *_ispc.h
  )
endforeach()


