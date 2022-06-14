## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(ospray_mpi_common STATIC
    Collectives.cpp
    MPIBcastFabric.cpp
    MPICommon.cpp
    ObjectHandle.cpp
    maml/maml.cpp
    maml/Context.cpp
    Profiling.cpp
)

target_link_libraries(ospray_mpi_common
  PRIVATE
    Snappy::snappy
  PUBLIC
    rkcommon::rkcommon
    ospray_module_cpu
    MPI::MPI_CXX
)

target_include_directories(ospray_mpi_common
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
)

if (OSPRAY_MPI_ENABLE_PROFILING)
  target_compile_definitions(ospray_mpi_common
    PUBLIC
      -DENABLE_PROFILING
  )
endif()

