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

set(COMPONENT_NAME OpenImageDenoise)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

set(OIDN_PATH "${COMPONENT_PATH}")
if (NOT WIN32)
  set(OIDN_PATH "${OIDN_PATH}/lib/cmake/OpenImageDenoise")
endif()

if (BUILD_OIDN_FROM_SOURCE)
  ExternalProject_Add(${COMPONENT_NAME}
    PREFIX ${COMPONENT_NAME}
    DOWNLOAD_DIR ${COMPONENT_NAME}
    STAMP_DIR ${COMPONENT_NAME}/stamp
    SOURCE_DIR ${COMPONENT_NAME}/src
    BINARY_DIR ${COMPONENT_NAME}/build
    GIT_REPOSITORY https://github.com/OpenImageDenoise/oidn
    GIT_TAG v${BUILD_OIDN_VERSION}
    GIT_SHALLOW ON
    CMAKE_ARGS
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
      -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
      -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
      -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
      -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
      -DCMAKE_BUILD_TYPE=Release
      -DTBB_ROOT=${TBB_PATH}
    BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
    BUILD_ALWAYS OFF
  )

  if (BUILD_TBB_FROM_SOURCE)
    ExternalProject_Add_StepDependencies(${COMPONENT_NAME} configure tbb)
  endif()
else()
  if (APPLE)
    set(OIDN_URL "https://github.com/OpenImageDenoise/oidn/releases/download/v${BUILD_OIDN_VERSION}/oidn-${BUILD_OIDN_VERSION}.x86_64.macos.tar.gz")
  elseif (WIN32)
    set(OIDN_URL "https://github.com/OpenImageDenoise/oidn/releases/download/v${BUILD_OIDN_VERSION}/oidn-${BUILD_OIDN_VERSION}.x64.vc14.windows.zip")
  else()
    set(OIDN_URL "https://github.com/OpenImageDenoise/oidn/releases/download/v${BUILD_OIDN_VERSION}/oidn-${BUILD_OIDN_VERSION}.x86_64.linux.tar.gz")
  endif()

  ExternalProject_Add(${COMPONENT_NAME}
    PREFIX ${COMPONENT_NAME}
    DOWNLOAD_DIR ${COMPONENT_NAME}
    STAMP_DIR ${COMPONENT_NAME}/stamp
    SOURCE_DIR ${COMPONENT_NAME}/src
    BINARY_DIR ${COMPONENT_NAME}
    URL ${OIDN_URL}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E copy_directory
      <SOURCE_DIR>/
      ${COMPONENT_PATH}
    BUILD_ALWAYS OFF
  )
endif()

