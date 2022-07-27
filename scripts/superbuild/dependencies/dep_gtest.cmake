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
  URL "https://github.com/google/googletest/archive/refs/tags/release-1.10.0.zip"
  URL_HASH "SHA256=94c634d499558a76fa649edb13721dce6e98fb1e7018dfaeba3cd7a083945e91"

#   # Skip updating on subsequent builds (faster)
  UPDATE_COMMAND ""

  CMAKE_ARGS
    -Dgtest_force_shared_crt=ON
    -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
    -DCMAKE_BUILD_TYPE=${DEPENDENCIES_BUILD_TYPE}
    -DBUILD_GMOCK=OFF
)

list(APPEND CMAKE_PREFIX_PATH ${COMPONENT_PATH})
string(REPLACE ";" "|" CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
