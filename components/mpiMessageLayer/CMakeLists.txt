## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
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

set(MAML_LIBRARY ospray_mpi_maml)

ospray_configure_mpi()

ospray_build_component(snappy EXCLUDE_FROM_ALL)
ospray_build_component(mpiCommon)

# ------------------------------------------------------------------
# The MAML library itself
# ------------------------------------------------------------------
ospray_create_library(${MAML_LIBRARY}
  maml/maml.cpp
  maml/Context.cpp
LINK
  ospray_mpi_common
  snappy
COMPONENT mpi
)

set(MAML_SDK_INSTALL_LOC
  ../maml #NOTE: this is "next" to the SDK/ directory
)

ospray_install_sdk_headers(
  maml/maml.h
  DESTINATION ${MAML_SDK_INSTALL_LOC}
)

# ------------------------------------------------------------------
# tests and examples
# ------------------------------------------------------------------

include_directories(${CMAKE_CURRENT_LIST_DIR})

ospray_create_test(mamlTest
  apps/mamlTest.cpp
  LINK
  ${MAML_LIBRARY}
)

ospray_create_test(mamlTestMessageSwarm
  apps/testMessageSwarm.cpp
  LINK
  ${MAML_LIBRARY}
)
