## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMPONENT_NAME ispc)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

if (APPLE)
  set(ISPC_URL http://sdvis.org/ospray/download/dependencies/osx/ispc-v1.12.0-macOS.tar.gz)
elseif(WIN32)
  set(ISPC_URL http://sdvis.org/ospray/download/dependencies/win/ispc-v1.12.0-windows.zip)
else()
  set(ISPC_URL http://sdvis.org/ospray/download/dependencies/linux/ispc-v1.12.0-linux.tar.gz)
endif()

ExternalProject_Add(${COMPONENT_NAME}
  PREFIX ${COMPONENT_NAME}
  STAMP_DIR ${COMPONENT_NAME}/stamp
  SOURCE_DIR ${COMPONENT_NAME}/src
  BINARY_DIR ${COMPONENT_NAME}
  URL ${ISPC_URL}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    <SOURCE_DIR>/bin/ispc${CMAKE_EXECUTABLE_SUFFIX}
    ${COMPONENT_PATH}/bin/ispc${CMAKE_EXECUTABLE_SUFFIX}
  BUILD_ALWAYS OFF
)

set(ISPC_PATH "${COMPONENT_PATH}/bin/ispc${CMAKE_EXECUTABLE_SUFFIX}")
