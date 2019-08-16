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

set(EMBREE_PATH "${INSTALL_DIR_ABSOLUTE}/embree")

if (BUILD_EMBREE_FROM_SOURCE)
  ExternalProject_Add(embree
    PREFIX embree
    DOWNLOAD_DIR embree
    STAMP_DIR embree/stamp
    SOURCE_DIR embree/src
    BINARY_DIR embree/build
    INSTALL_DIR ${INSTALL_DIR_ABSOLUTE}/embree
    URL "https://github.com/embree/embree/archive/v${BUILD_EMBREE_VERSION}.zip"
    CMAKE_ARGS
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      -DEMBREE_TUTORIALS=OFF
      -DEMBREE_TBB_ROOT=${TBB_INSTALL_LOCATION}
      -DEMBREE_ISPC_EXECUTABLE=${ISPC_PATH}
      -DCMAKE_BUILD_TYPE=Release
      -DBUILD_TESTING=OFF
    BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
    BUILD_ALWAYS OFF
  )

  if (NOT WIN32)
    set(EMBREE_PATH "${EMBREE_PATH}/${CMAKE_INSTALL_LIBDIR}/cmake/embree-${BUILD_EMBREE_VERSION}")
  endif()

  ExternalProject_Add_StepDependencies(embree configure ispc tbb)
else()
  if (APPLE)
    set(EMBREE_URL "https://github.com/embree/embree/releases/download/v${BUILD_EMBREE_VERSION}/embree-${BUILD_EMBREE_VERSION}.x86_64.macosx.tar.gz")
  elseif (WIN32)
    set(EMBREE_URL "https://github.com/embree/embree/releases/download/v${BUILD_EMBREE_VERSION}/embree-${BUILD_EMBREE_VERSION}.x64.vc14.windows.zip")
  else()
    set(EMBREE_URL "https://github.com/embree/embree/releases/download/v${BUILD_EMBREE_VERSION}/embree-${BUILD_EMBREE_VERSION}.x86_64.linux.tar.gz")
  endif()

  ExternalProject_Add(embree
    PREFIX embree
    DOWNLOAD_DIR embree
    SOURCE_DIR ${INSTALL_DIR_ABSOLUTE}/embree
    URL ${EMBREE_URL}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    BUILD_ALWAYS OFF
  )
endif()
