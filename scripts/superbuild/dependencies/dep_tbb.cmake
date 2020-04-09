## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMPONENT_NAME tbb)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

if (APPLE)
  set(TBB_URL "http://github.com/intel/tbb/releases/download/v${TBB_VERSION}/tbb-${TBB_VERSION}-mac.tgz")
elseif (WIN32)
  set(TBB_URL "http://github.com/intel/tbb/releases/download/v${TBB_VERSION}/tbb-${TBB_VERSION}-win.zip")
else()
  set(TBB_URL "http://github.com/intel/tbb/releases/download/v${TBB_VERSION}/tbb-${TBB_VERSION}-lin.tgz")
endif()

ExternalProject_Add(${COMPONENT_NAME}
  PREFIX ${COMPONENT_NAME}
  DOWNLOAD_DIR ${COMPONENT_NAME}
  STAMP_DIR ${COMPONENT_NAME}/stamp
  SOURCE_DIR ${COMPONENT_NAME}/src
  BINARY_DIR ${COMPONENT_NAME}
  URL ${TBB_URL}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND "${CMAKE_COMMAND}" -E copy_directory
    <SOURCE_DIR>/tbb
    ${COMPONENT_PATH}
  BUILD_ALWAYS OFF
)

set(TBB_PATH "${COMPONENT_PATH}")
