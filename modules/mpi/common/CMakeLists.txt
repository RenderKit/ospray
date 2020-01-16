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

add_library(ospray_mpi_common SHARED
    Collectives.cpp
    MPIBcastFabric.cpp
    MPICommon.cpp
    SocketBcastFabric.cpp
    ObjectHandle.cpp
    maml/maml.cpp
    maml/Context.cpp
)

ospray_install_library(ospray_mpi_common mpi)

target_link_libraries(ospray_mpi_common
  PRIVATE
    snappy
  PUBLIC
    ${MPI_CXX_LIBRARIES}
    ospcommon::ospcommon
)

target_include_directories(ospray_mpi_common
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ospray/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ospray>
    $<BUILD_INTERFACE:${OSPRay_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/mpiCommon>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/mpiCommon/maml>
    ${OSPRAY_MPI_INCLUDES}
)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray
  COMPONENT devel
  FILES_MATCHING
  PATTERN *.h
)

