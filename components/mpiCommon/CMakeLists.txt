## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
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

OSPRAY_CONFIGURE_MPI()

OSPRAY_CREATE_LIBRARY(ospray_mpi_common

  MPICommon.cpp
  MPIBcastFabric.cpp

LINK

  ${MPI_C_LIBRARIES}
  ospray_common
  ospray

COMPONENT mpi
)

SET(MPICOMMON_SDK_INSTALL_LOC
  ../mpiCommon #NOTE: this is "next" to the SDK/ directory
)

OSPRAY_INSTALL_SDK_HEADERS(
  MPICommon.h
  MPIBcastFabric.h
  DESTINATION ${MPICOMMON_SDK_INSTALL_LOC}
)
