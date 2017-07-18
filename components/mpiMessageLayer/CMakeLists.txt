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

SET(MAML_LIBRARY ospray_mpi_maml)

OSPRAY_CONFIGURE_MPI()

OSPRAY_BUILD_COMPONENT(mpiCommon)

# ------------------------------------------------------------------
# The MAML library itself
# ------------------------------------------------------------------
OSPRAY_CREATE_LIBRARY(${MAML_LIBRARY}
  maml/maml.cpp
  maml/Context.cpp
LINK
  ospray_mpi_common
COMPONENT mpi
)

SET(MAML_SDK_INSTALL_LOC
  ../maml #NOTE: this is "next" to the SDK/ directory
)

OSPRAY_INSTALL_SDK_HEADERS(
  maml/maml.h
  DESTINATION ${MAML_SDK_INSTALL_LOC}
)

# ------------------------------------------------------------------
# tests and examples
# ------------------------------------------------------------------

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR})

OSPRAY_CREATE_TEST(mamlTest
  apps/mamlTest.cpp
  LINK
  ${MAML_LIBRARY}
)

OSPRAY_CREATE_TEST(mamlTestMessageSwarm
  apps/testMessageSwarm.cpp
  LINK
  ${MAML_LIBRARY}
)

