## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.1)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(CMAKE_C_STANDARD   99)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(ospray_module_mpi VERSION 2.5.0 LANGUAGES C CXX)

configure_file(
  ${CMAKE_SOURCE_DIR}/ospray/version.h.in
  ${CMAKE_BINARY_DIR}/ospray/version.h
  @ONLY
)
set(OSPRAY_MPI_RESOURCE ${CMAKE_SOURCE_DIR}/ospray/ospray.rc)

include(GNUInstallDirs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

find_package(MPI REQUIRED)
find_package(ospray 2.2 REQUIRED)
include(${OSPRAY_SDK_USE_FILE})

rkcommon_configure_build_type()

option(BUILD_TUTORIALS "Enable MPI module sample apps" OFF)
option(ENABLE_PROFILING "Enable MPI module internal profiling" OFF)

add_subdirectory(common)
add_subdirectory(external)
add_subdirectory(ospray)

if (BUILD_TUTORIALS)
  add_subdirectory(tutorials)
endif()

