## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## Options ##

set(OSPRAY_MPI_COMPRESSION_THRESHOLD 16 CACHE STRING
  "Min. number of ranks to enable message compression at (number)" )
mark_as_advanced(OSPRAY_MPI_COMPRESSION_THRESHOLD)

## Build the MPI Offload module ##

add_library(ospray_module_mpi_offload SHARED
  OffloadModuleInit.cpp
  MPIOffloadWorker.cpp
  MPIOffloadDevice.cpp

  common/OSPWork.cpp
  common/ApplicationData.cpp
)

target_link_libraries(ospray_module_mpi_offload
  PRIVATE
    rkcommon::rkcommon
    ospray_mpi_common
)

target_include_directories(ospray_module_mpi_offload
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

target_compile_definitions(ospray_module_mpi_offload PUBLIC
  OSP_MPI_COMPRESSION_THRESHOLD=${OSPRAY_MPI_COMPRESSION_THRESHOLD}
  # We don't want an instance of ObjectFactory static table in this library
  # so we have to include it with import define so the table will be imported
  # from 'ospray' library
  OBJECTFACTORY_IMPORT)

ospray_install_library(ospray_module_mpi_offload mpi)

## Build the worker executable ##

add_executable(ospray_mpi_worker
  ${OSPRAY_RESOURCE} mpi_offload_worker_main.cpp)

target_link_libraries(ospray_mpi_worker PRIVATE ospray)

install(TARGETS ospray_mpi_worker
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT mpi)

ospray_sign_target(ospray_mpi_worker)

## Build the MPI Distributed CPU module ##

set(OSPRAY_MPI_DISTRIBUTED_CPP_SOURCES
  ${OSPRAY_RESOURCE}

  MPIDistributedDevice.cpp
  DistributedModuleInit.cpp

  common/DistributedWorld.cpp
  common/Messaging.cpp
  common/DynamicLoadBalancer.cpp

  fb/DistributedFrameBuffer.cpp
  fb/DistributedFrameBuffer_TileMessages.cpp
  fb/TileOperation.cpp

  render/DistributedLoadBalancer.cpp
  render/WriteMultipleTileOperation.cpp
  render/distributed/DistributedRaycast.cpp
  render/distributed/DistributedRenderer.cpp
  render/distributed/AlphaCompositeTileOperation.cpp
)

set(OSPRAY_MPI_DISTRIBUTED_ISPC_SOURCES
  MPIDistributedDevice.ispc
  fb/DistributedFrameBuffer.ispc
  render/distributed/DistributedRaycast.ispc
  render/distributed/DistributedRenderer.ispc
)

include_directories_ispc(
  ${PROJECT_SOURCE_DIR}/ospray/include
  ${PROJECT_SOURCE_DIR}/ospray
  ${PROJECT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${RKCOMMON_INCLUDE_DIRS}
  ${EMBREE_INCLUDE_DIRS}
)

add_definitions_ispc(
  -DOSPRAY_BEGIN_ISPC_NAMESPACE=
  -DOSPRAY_END_ISPC_NAMESPACE=
  -DSYCL_EXTERNAL=
)

if (OSPRAY_ENABLE_VOLUMES)
  include_directories_ispc(
    ${OPENVKL_INCLUDE_DIRS}
  )
  # Keep the NOTFOUND out of the ISPC include dirs list, because CMake will
  # see the variable ending with NOTFOUND and evaluate it to false and not
  # pass our include directories
  if (OPENVKL_CPU_DEVICE_INCLUDE_DIRS)
    include_directories_ispc(
      ${OPENVKL_CPU_DEVICE_INCLUDE_DIRS}
    )
  endif()

  add_definitions_ispc(
    -DOSPRAY_ENABLE_VOLUMES
  )
endif()

add_library(ospray_module_mpi_distributed_cpu SHARED)

ispc_target_add_sources(ospray_module_mpi_distributed_cpu
    ${OSPRAY_MPI_DISTRIBUTED_CPP_SOURCES}
    ${OSPRAY_MPI_DISTRIBUTED_ISPC_SOURCES}
)

target_link_libraries(ospray_module_mpi_distributed_cpu
  PRIVATE
    Snappy::snappy
    rkcommon::rkcommon
    ospray_mpi_common
    ospray_module_cpu
)

target_include_directories(ospray_module_mpi_distributed_cpu
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    # For ISPC module generated ispc headers
    ${PROJECT_BINARY_DIR}/ospray/
)

# Note: GPU hybrid code defines are brought in for us by linking the CPU module
target_compile_definitions(ospray_module_mpi_distributed_cpu PRIVATE
  OSP_MPI_COMPRESSION_THRESHOLD=${OSPRAY_MPI_COMPRESSION_THRESHOLD}
  # We don't want an instance of ObjectFactory static table in this library
  # so we have to include it with import define so the table will be imported
  # from 'ospray' library
  OBJECTFACTORY_IMPORT
)

ospray_install_library(ospray_module_mpi_distributed_cpu mpi)

## Build the MPI Distributed GPU module ##

if (OSPRAY_MODULE_GPU)
  include(${ISPCRT_DIR}/interop.cmake)

  ospray_configure_dpcpp_target()

  # Have CMake's build infrastructure treat the ISPC files as C++ for SYCL
  set_source_files_properties(${OSPRAY_MPI_DISTRIBUTED_ISPC_SOURCES} PROPERTIES
      LANGUAGE CXX)

  add_library(ospray_module_mpi_distributed_gpu SHARED
    ${OSPRAY_MPI_DISTRIBUTED_CPP_SOURCES}
    ${OSPRAY_MPI_DISTRIBUTED_ISPC_SOURCES})

  set_target_properties(ospray_module_mpi_distributed_gpu PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

  target_link_libraries(ospray_module_mpi_distributed_gpu
  PRIVATE
    Snappy::snappy
    rkcommon::rkcommon
    ospray_mpi_common
    ospray_module_gpu_kernels
    ospray_module_gpu
  )

  target_include_directories(ospray_module_mpi_distributed_gpu
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}
      # For ISPC module generated ispc headers
      ${PROJECT_BINARY_DIR}/ospray/
  )

  target_compile_options(ospray_module_mpi_distributed_gpu
  PRIVATE
    -x c++
  )

  # Note: GPU hybrid code defines are brought in for us by linking the CPU module
  target_compile_definitions(ospray_module_mpi_distributed_gpu PRIVATE
    OSP_MPI_COMPRESSION_THRESHOLD=${OSPRAY_MPI_COMPRESSION_THRESHOLD}
    # We don't want an instance of ObjectFactory static table in this library
    # so we have to include it with import define so the table will be imported
    # from 'ospray' library
    OBJECTFACTORY_IMPORT
  )

  ospray_install_library(ospray_module_mpi_distributed_gpu lib)
endif()

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


