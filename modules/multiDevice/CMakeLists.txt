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

set(OSPRAY_MULTIDEVICE_CPP_SOURCES
  ${OSPRAY_RESOURCE}

  MultiDevice.cpp
  MultiDeviceLoadBalancer.cpp
  MultiDeviceModuleInit.cpp 
)

include_directories_ispc(
  ${PROJECT_SOURCE_DIR}/ospray/include
  ${PROJECT_SOURCE_DIR}/ospray
  ${PROJECT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${RKCOMMON_INCLUDE_DIRS}
  ${EMBREE_INCLUDE_DIRS}
)

add_definitions_ispc(
  OSPRAY_BEGIN_ISPC_NAMESPACE=
  OSPRAY_END_ISPC_NAMESPACE=
  SYCL_EXTERNAL=
  __noinline=
  OSPRAY_ENABLE_VOLUMES
)


add_library(ospray_module_multidevice_cpu SHARED)

ispc_target_add_sources(ospray_module_multidevice_cpu
  ${OSPRAY_MULTIDEVICE_CPP_SOURCES}
)

target_link_libraries(ospray_module_multidevice_cpu
  PRIVATE
    rkcommon::rkcommon
    ospray_module_cpu
)

target_include_directories(ospray_module_multidevice_cpu
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    # For ISPC module generated ispc headers
    ${PROJECT_BINARY_DIR}/ospray/
)

target_compile_definitions(ospray_module_multidevice_cpu PRIVATE
  OBJECTFACTORY_IMPORT
)

ospray_install_library(ospray_module_multidevice_cpu lib)

if (OSPRAY_MODULE_GPU)
  include(${ISPCRT_DIR}/interop.cmake)
  if (NOT "${CMAKE_CXX_COMPILER}" STREQUAL "${DPCPP_COMPILER}")
    message(FATAL_ERROR "CMAKE_CXX_COMPILER should be set to '${DPCPP_COMPILER}' for SYCL")
  endif()

  ospray_configure_dpcpp_target()

  add_library(ospray_module_multidevice_gpu SHARED
    ${OSPRAY_MULTIDEVICE_CPP_SOURCES})

  set_target_properties(ospray_module_multidevice_gpu PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

  target_link_libraries(ospray_module_multidevice_gpu
  PRIVATE
    rkcommon::rkcommon
    ospray_module_gpu
  )

  target_include_directories(ospray_module_multidevice_gpu
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    # For ISPC module generated ispc headers
    ${PROJECT_BINARY_DIR}/ospray/
  )

  target_compile_options(ospray_module_multidevice_gpu
  PRIVATE
    -x c++
  )

  target_compile_definitions(ospray_module_multidevice_gpu PRIVATE
    OBJECTFACTORY_IMPORT
  )

  ospray_install_library(ospray_module_multidevice_gpu lib)

endif()
