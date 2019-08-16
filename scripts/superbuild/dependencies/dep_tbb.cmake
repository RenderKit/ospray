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

ExternalProject_Add(tbb
  PREFIX tbb
  DOWNLOAD_DIR tbb
  STAMP_DIR tbb/stamp
  SOURCE_DIR tbb/src
  BINARY_DIR tbb/build
  INSTALL_DIR ${INSTALL_DIR_ABSOLUTE}/tbb
  URL "https://github.com/wjakob/tbb/archive/master.zip"
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DCMAKE_BUILD_TYPE=Release
    -DTBB_BUILD_TESTS=OFF
    -DTBB_BUILD_SHARED=ON
    -DTBB_BUILD_STATIC=OFF
  BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
  BUILD_ALWAYS OFF
)

set(TBB_INSTALL_LOCATION "${INSTALL_DIR_ABSOLUTE}/tbb")
