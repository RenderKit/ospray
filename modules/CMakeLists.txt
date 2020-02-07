## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

get_target_property(OSPRAY_ISPC_MODULE_INCLUDE_DIRS ospray_module_ispc
  INTERFACE_INCLUDE_DIRECTORIES)

ispc_include_directories(${OSPRAY_ISPC_MODULE_INCLUDE_DIRS})

add_all_subdirectories()
