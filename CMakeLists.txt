## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.1)

##############################################################
# Language setup
##############################################################

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(CMAKE_C_STANDARD   99)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


include(../../../cmake/ensight_cmake_config.cmake)
set(embree_DIR "${APEX_PLATFORM_DIR}intel_embree-3.10.0/" CACHE PATH "embree_DIR" FORCE)
set(rkcommon_DIR "${APEX_PLATFORM_DIR}intl_rkcommon-1.6.0/lib/cmake/rkcommon-1.6.0/" CACHE PATH "rkcommon_DIR" FORCE)
set(openvkl_DIR "${APEX_PLATFORM_DIR}intl_openvkl-0.11.0/lib/cmake/openvkl-0.11.0/" CACHE PATH "openvkl_DIR" FORCE)
set(CMAKE_INSTALL_PREFIX "${APEX_PLATFORM_DIR}intel_ospray-2.1.1/" CACHE PATH "CMAKE_INSTALL_PREFIX" FORCE)

##############################################################
# Establish project
##############################################################

include(cmake/ospray_version.cmake)

project(OSPRay VERSION ${OSPRAY_VERSION} LANGUAGES C CXX)

##############################################################
# CMake modules and macro files
##############################################################

list(APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake
  ${PROJECT_SOURCE_DIR}/cmake/compiler
)

include(ospray_macros)
include(ospray_options)
include(package)
include(ispc)

if (OSPRAY_INSTALL_DEPENDENCIES)
  include(ospray_redistribute_deps)
endif()

##############################################################
# Add library and executable targets
##############################################################

## Main OSPRay library ##
add_subdirectory(ospray)

## OSPRay sample apps ##
add_subdirectory(apps)

## Modules ##
add_subdirectory(modules)

## Regression test images target ##
add_subdirectory(test_image_data)

## Clang-format target ##
if (OSPRAY_ENABLE_TARGET_CLANGFORMAT)
  include(clang-format)
endif()

# Must be last
include(CPack)
