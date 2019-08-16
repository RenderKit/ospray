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

ExternalProject_Add(oidn
  PREFIX oidn
  DOWNLOAD_DIR oidn
  STAMP_DIR oidn/stamp
  SOURCE_DIR oidn/src
  BINARY_DIR oidn/build
  INSTALL_DIR ${INSTALL_DIR_ABSOLUTE}/OpenImageDenoise
  GIT_REPOSITORY https://github.com/OpenImageDenoise/oidn
  GIT_TAG v${BUILD_OIDN_VERSION}
  GIT_SHALLOW ON
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_BUILD_TYPE=Release
    -DTBB_ROOT=${TBB_ROOT}
  BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
  BUILD_ALWAYS OFF
)

set(OIDN_PATH "${INSTALL_DIR_ABSOLUTE}/OpenImageDenoise")
if (NOT WIN32)
  set(OIDN_PATH "${OIDN_PATH}/${CMAKE_INSTALL_LIBDIR}/cmake/OpenImageDenoise")
endif()

ExternalProject_Add_StepDependencies(oidn configure tbb ispc)
