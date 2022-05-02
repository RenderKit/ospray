## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_subdirectory(cpu)

if(OSPRAY_MODULE_CPU)
  get_target_property(OSPRAY_CPU_MODULE_INCLUDE_DIRS ospray_module_cpu
    INTERFACE_INCLUDE_DIRECTORIES)
  ispc_include_directories(${OSPRAY_CPU_MODULE_INCLUDE_DIRS})
endif()

add_all_subdirectories_except(cpu)
