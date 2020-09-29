## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMPONENT_NAME oidn)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

if (BUILD_OIDN_FROM_SOURCE)
  ExternalProject_Add(${COMPONENT_NAME}
    PREFIX ${COMPONENT_NAME}
    DOWNLOAD_DIR ${COMPONENT_NAME}
    STAMP_DIR ${COMPONENT_NAME}/stamp
    SOURCE_DIR ${COMPONENT_NAME}/src
    BINARY_DIR ${COMPONENT_NAME}/build
    LIST_SEPARATOR | # Use the alternate list separator
    GIT_REPOSITORY "https://www.github.com/OpenImageDenoise/oidn.git"
    GIT_SHALLOW ON
    CMAKE_ARGS
      -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
      -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
      -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
      -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
      -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
      $<$<BOOL:${DOWNLOAD_TBB}>:-DTBB_ROOT=${TBB_PATH}>
      $<$<BOOL:${DOWNLOAD_ISPC}>:-DISPC_EXECUTABLE=${ISPC_PATH}>
      -DCMAKE_BUILD_TYPE=${DEPENDENCIES_BUILD_TYPE}
      -DOIDN_APPS=OFF
    BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
    BUILD_ALWAYS ${ALWAYS_REBUILD}
  )

  if (DOWNLOAD_TBB)
    ExternalProject_Add_StepDependencies(${COMPONENT_NAME} configure tbb)
  endif()
  if (DOWNLOAD_ISPC)
    ExternalProject_Add_StepDependencies(${COMPONENT_NAME} configure ispc)
  endif()
else()
  string(REPLACE "v" "" OIDN_VERSION_NUMBER ${BUILD_OIDN_VERSION})

  if (APPLE)
    set(OIDN_URL "http://github.com/OpenImageDenoise/oidn/releases/download/${BUILD_OIDN_VERSION}/oidn-${OIDN_VERSION_NUMBER}.x86_64.macos.tar.gz")
  elseif (WIN32)
    set(OIDN_URL "http://github.com/OpenImageDenoise/oidn/releases/download/${BUILD_OIDN_VERSION}/oidn-${OIDN_VERSION_NUMBER}.x64.vc14.windows.zip")
  else()
    set(OIDN_URL "http://github.com/OpenImageDenoise/oidn/releases/download/${BUILD_OIDN_VERSION}/oidn-${OIDN_VERSION_NUMBER}.x86_64.linux.tar.gz")
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

list(APPEND CMAKE_PREFIX_PATH ${COMPONENT_PATH})
string(REPLACE ";" "|" CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")

