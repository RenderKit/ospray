## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMPONENT_NAME benchmark)

if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
else()
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
endif()

ExternalProject_Add(${COMPONENT_NAME}
  URL "https://github.com/google/benchmark/archive/refs/tags/v1.5.5.zip"
  URL_HASH "SHA256=30f2e5156de241789d772dd8b130c1cb5d33473cc2f29e4008eab680df7bd1f0"

  # Skip updating on subsequent builds (faster)
  UPDATE_COMMAND ""

  DEPENDS gtest

  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
    -DBENCHMARK_ENABLE_TESTING=OFF
    -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
)

list(APPEND CMAKE_PREFIX_PATH ${COMPONENT_PATH})
string(REPLACE ";" "|" CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
