## Copyright 2009-2021 Intel Corporation
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
    snappy
  PUBLIC
    rkcommon::rkcommon
    ospray_sdk
    ospray_module_ispc
    ${MPI_CXX_LIBRARIES}
)

target_include_directories(ospray_mpi_common
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
    ${MPI_CXX_INCLUDE_PATH}
)

if (OSPRAY_MPI_ENABLE_PROFILING)
  target_compile_definitions(ospray_mpi_common
    PUBLIC
      -DENABLE_PROFILING
  )
endif()

