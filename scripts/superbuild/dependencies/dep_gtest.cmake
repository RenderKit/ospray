## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMPONENT_NAME gtest)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

# Google Test follows the "Live Head" philosophy and therefore recommends
# using the latest commit to 'master' 
ExternalProject_Add(${COMPONENT_NAME}
  URL "https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip"
  URL_HASH "SHA256=63b9c77751a5b8f492486005f67533fdc58682b67476fcb91650be5958d5195a"

#   # Skip updating on subsequent builds (faster)
  UPDATE_COMMAND ""

  CMAKE_ARGS
    -Dgtest_force_shared_crt=ON
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_BUILD_TYPE=${DEPENDENCIES_BUILD_TYPE}
    -DBUILD_GMOCK=OFF
)

list(APPEND CMAKE_PREFIX_PATH ${COMPONENT_PATH})
string(REPLACE ";" "|" CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
