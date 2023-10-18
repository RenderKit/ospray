## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMPONENT_NAME snappy)

if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
else()
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
endif()

ExternalProject_Add(${COMPONENT_NAME}
  URL "https://github.com/google/snappy/archive/refs/tags/1.1.10.zip"
  URL_HASH "SHA256=3c6f7b07f92120ebbba5f7742f2cc2386fd46c53f5730322b7b90d4afc126fca"

  # `patch` is not available on all systems, so use `git apply` instead
  PATCH_COMMAND git init -q . && git apply -v -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/snappy-sign-compare.patch

  # Skip updating on subsequent builds (faster)
  UPDATE_COMMAND ""

  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DBUILD_SHARED_LIBS:BOOL=OFF
    -DSNAPPY_BUILD_TESTS:BOOL=OFF
    -DSNAPPY_BUILD_BENCHMARKS:BOOL=OFF
    -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
    -DCMAKE_BUILD_TYPE=${DEPENDENCIES_BUILD_TYPE}
)

list(APPEND CMAKE_PREFIX_PATH ${COMPONENT_PATH})
string(REPLACE ";" "|" CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
