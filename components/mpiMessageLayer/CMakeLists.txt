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

OSPRAY_BUILD_COMPONENT(mpiCommon)

OSPRAY_CONFIGURE_MPI()

SET(MAML_LIBRARY ospray_mpi_maml)

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

# ------------------------------------------------------------------
# tests and examples
# ------------------------------------------------------------------
OSPRAY_CREATE_OPTIONAL_APPLICATION(mamlTest
  apps/mamlTest.cpp
LINK
  ${MAML_LIBRARY}
)

OSPRAY_CREATE_OPTIONAL_APPLICATION(mamlTestMessageSwarm
  apps/testMessageSwarm.cpp
LINK
  ${MAML_LIBRARY}
)
