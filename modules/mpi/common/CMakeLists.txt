## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(ospray_mpi_common STATIC
    Collectives.cpp
    MPIBcastFabric.cpp
    MPICommon.cpp
    SocketBcastFabric.cpp
    ObjectHandle.cpp
    maml/maml.cpp
    maml/Context.cpp
    Profiling.cpp
)

target_link_libraries(ospray_mpi_common
  PRIVATE
    snappy
  PUBLIC
    ospray::ospray_sdk
    rkcommon::rkcommon
    ospray::ospray_module_ispc
    ${MPI_C_LIBRARIES}
)

target_include_directories(ospray_mpi_common
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/mpiCommon>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/mpiCommon/maml>
    ${MPI_C_INCLUDE_PATH}
)
