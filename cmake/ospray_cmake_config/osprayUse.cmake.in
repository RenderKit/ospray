## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# NOTE(jda) - Add CMake commands which will be invoked when find_package(ospray)
#             is called by client applications. For example, it may be useful
#             to automatically have an add_definitions(...) call for clients
#             which need certain preprocessor definitions based on how ospray
#             was built.

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${OSPRAY_CMAKE_ROOT})

include(ispc)

message(STATUS "Using found OSPRay ISA targets: ${OSPRAY_ISPC_TARGET_LIST}")

if (NOT TARGET openvkl::openvkl OR NOT TARGET embree)
  message(FATAL_ERROR "OSPRay pre-built binary redistributables cannot be
  extended with the SDK. Please build OSPRay from source to enable extending
  OSPRay!")
endif()

ispc_include_directories(
  ${OSPRAY_INCLUDE_DIR}
  ${OSPRAY_INCLUDE_DIR}/ospray/SDK
  ${RKCOMMON_INCLUDE_DIRS}
  ${EMBREE_INCLUDE_DIRS}
  ${OPENVKL_INCLUDE_DIRS}
)
