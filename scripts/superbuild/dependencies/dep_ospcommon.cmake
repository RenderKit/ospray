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

ExternalProject_Add(ospcommon
  PREFIX ospcommon
  DOWNLOAD_DIR ospcommon
  STAMP_DIR ospcommon/stamp
  SOURCE_DIR ospcommon/src
  BINARY_DIR ospcommon/build
  INSTALL_DIR ${INSTALL_DIR_ABSOLUTE}/ospcommon
  GIT_REPOSITORY https://github.com/ospray/ospcommon
  GIT_TAG master
  GIT_SHALLOW ON
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_BUILD_TYPE=Release
    -DOSPCOMMON_TBB_ROOT=${TBB_INSTALL_LOCATION}
  BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
  BUILD_ALWAYS OFF
)

set(OSPCOMMON_PATH "${INSTALL_DIR_ABSOLUTE}/ospcommon")
if (NOT WIN32)
  set(OSPCOMMON_PATH "${OSPCOMMON_PATH}/lib/cmake/ospcommon")
endif()

ExternalProject_Add_StepDependencies(ospcommon configure tbb)
