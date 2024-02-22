## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMPONENT_NAME oidn)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

if (OIDN_HASH)
  set(OIDN_URL_HASH URL_HASH SHA256=${OIDN_HASH})
endif()

if (BUILD_OIDN_FROM_SOURCE)
  if(${OIDN_VERSION} MATCHES "(^[0-9]+\.[0-9]+\.[0-9]+$)")
    set(OIDN_DEFAULT_URL "https://github.com/OpenImageDenoise/oidn/releases/download/v${OIDN_VERSION}/oidn-${OIDN_VERSION}.src.tar.gz")
  else()
    set(OIDN_DEFAULT_URL "https://www.github.com/OpenImageDenoise/oidn.git")
  endif()
    set(OIDN_URL ${OIDN_DEFAULT_URL} CACHE STRING "Location to get OpenImageDenoise source from")
  if (${OIDN_URL} MATCHES ".*\.src\.tar\.gz$")
    set(OIDN_CLONE_URL URL ${OIDN_URL})
  else()
    set(OIDN_CLONE_URL GIT_REPOSITORY ${OIDN_URL} GIT_TAG ${OIDN_VERSION})
  endif()

  ExternalProject_Add(${COMPONENT_NAME}
    PREFIX ${COMPONENT_NAME}
    DOWNLOAD_DIR ${COMPONENT_NAME}
    STAMP_DIR ${COMPONENT_NAME}/stamp
    SOURCE_DIR ${COMPONENT_NAME}/src
    BINARY_DIR ${COMPONENT_NAME}/build
    LIST_SEPARATOR | # Use the alternate list separator
    ${OIDN_CLONE_URL}
    ${OIDN_URL_HASH}
    GIT_SHALLOW ON
    CMAKE_ARGS
      -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
      -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
      -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
      -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
      -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
      -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
      $<$<BOOL:${DOWNLOAD_TBB}>:-DTBB_ROOT=${TBB_PATH}>
      $<$<BOOL:${DOWNLOAD_ISPC}>:-DISPC_EXECUTABLE=${ISPC_PATH}>
      -DCMAKE_BUILD_TYPE=Release # XXX debug builds are currently broken
      -DOIDN_APPS=OFF
      -DOIDN_ZIP_MODE=ON # to set install RPATH
      -DOIDN_DEVICE_SYCL=${BUILD_GPU_SUPPORT}
      -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
      -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
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

  if (APPLE)
    set(OIDN_OSSUFFIX "${CMAKE_SYSTEM_PROCESSOR}.macos.tar.gz")
  elseif (WIN32)
    set(OIDN_OSSUFFIX "x64.windows.zip")
  else()
    set(OIDN_OSSUFFIX "x86_64.linux.tar.gz")
  endif()
  set(OIDN_URL "https://github.com/OpenImageDenoise/oidn/releases/download/v${OIDN_VERSION}/oidn-${OIDN_VERSION}.${OIDN_OSSUFFIX}")

  ExternalProject_Add(${COMPONENT_NAME}
    PREFIX ${COMPONENT_NAME}
    DOWNLOAD_DIR ${COMPONENT_NAME}
    STAMP_DIR ${COMPONENT_NAME}/stamp
    SOURCE_DIR ${COMPONENT_NAME}/src
    BINARY_DIR ${COMPONENT_NAME}
    URL ${OIDN_URL}
    ${OIDN_URL_HASH}
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

