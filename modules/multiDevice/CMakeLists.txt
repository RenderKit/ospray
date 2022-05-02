## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_dependent_option(
  OSPRAY_MODULE_MULTIDEVICE
  "Build the multidevice module"
  OFF
  OSPRAY_MODULE_CPU
  OFF
)

if (NOT OSPRAY_MODULE_MULTIDEVICE)
  return()
endif()

rkcommon_configure_build_type()

ispc_include_directories(
  ${PROJECT_SOURCE_DIR}/ospray/include
  ${PROJECT_SOURCE_DIR}/ospray
  ${PROJECT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${RKCOMMON_INCLUDE_DIRS}
  ${EMBREE_INCLUDE_DIRS}
)

add_library(ospray_module_multidevice SHARED)

ispc_target_add_sources(ospray_module_multidevice
  ${OSPRAY_RESOURCE}

  MultiDevice.cpp
  MultiDeviceLoadBalancer.cpp
  ModuleInit.cpp
)

target_link_libraries(ospray_module_multidevice
  PRIVATE
    rkcommon::rkcommon
    ospray_module_cpu
)

target_include_directories(ospray_module_multidevice
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    # For ISPC module generated ispc headers
    ${PROJECT_BINARY_DIR}/ospray/
)

ospray_install_library(ospray_module_multidevice lib)

