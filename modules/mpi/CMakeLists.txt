## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_dependent_option(
  OSPRAY_MODULE_MPI
  "Build the MPI module"
  OFF
  OSPRAY_MODULE_CPU
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
if(NOT TARGET MPI::MPI_CXX) # pre CMake 3.9
  add_library(MPI::MPI_CXX IMPORTED INTERFACE)
  set_property(TARGET MPI::MPI_CXX PROPERTY INTERFACE_COMPILE_OPTIONS ${MPI_CXX_COMPILE_FLAGS})
  set_property(TARGET MPI::MPI_CXX PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${MPI_CXX_INCLUDE_PATH}")
  set_property(TARGET MPI::MPI_CXX PROPERTY INTERFACE_LINK_LIBRARIES ${MPI_CXX_LINK_FLAGS} ${MPI_CXX_LIBRARIES})
endif()

find_package(Snappy REQUIRED)

rkcommon_configure_build_type()

add_subdirectory(common)
add_subdirectory(external)
add_subdirectory(ospray)

if (OSPRAY_ENABLE_APPS_EXAMPLES)
  add_subdirectory(tutorials)
endif()

if (OSPRAY_ENABLE_APPS_TESTING)
    add_subdirectory(testing)
endif()
