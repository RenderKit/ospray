## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMPONENT_NAME ispc)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

set(ISPC_BASE_URL "https://github.com/ispc/ispc/releases/download")
if (APPLE)
  set(ISPC_URL ${ISPC_BASE_URL}/${ISPC_VERSION}/ispc-${ISPC_VERSION}-macOS.tar.gz)
elseif(WIN32)
  set(ISPC_URL ${ISPC_BASE_URL}/${ISPC_VERSION}/ispc-${ISPC_VERSION}-windows.zip)
else()
  if (ISPC_VERSION STREQUAL "v1.12.0")
    # handle typo(?) in v1.12.0 in linux package name
    set(ISPC_URL ${ISPC_BASE_URL}/${ISPC_VERSION}/ispc-${ISPC_VERSION}b-linux.tar.gz)
  else()
    set(ISPC_URL ${ISPC_BASE_URL}/${ISPC_VERSION}/ispc-${ISPC_VERSION}-linux.tar.gz)
  endif()
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
