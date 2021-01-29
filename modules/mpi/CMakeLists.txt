## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_dependent_option(
  OSPRAY_MODULE_MPI
  "Build the MPI module (Requires ISPC module)"
  OFF
  OSPRAY_ENABLE_MODULES
  OFF
)

cmake_dependent_option(
  OSPRAY_MPI_BUILD_TUTORIALS
  "Enable MPI module sample apps"
  ON
  "OSPRAY_MODULE_MPI;OSPRAY_ENABLE_APPS"
  OFF
)

cmake_dependent_option(
  OSPRAY_MPI_ENABLE_PROFILING
  "Enable MPI module internal profiling"
  OFF
  OSPRAY_MODULE_MPI
  OFF
)

if (NOT OSPRAY_MODULE_MPI)
  return()
endif()

find_package(MPI REQUIRED)

rkcommon_configure_build_type()

add_subdirectory(common)
add_subdirectory(external)
add_subdirectory(ospray)

if (OSPRAY_MPI_BUILD_TUTORIALS)
  add_subdirectory(tutorials)
endif()

