## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (NOT OSPRAY_ENABLE_APPS_BENCHMARK AND
    NOT OSPRAY_ENABLE_APPS_EXAMPLES AND
    NOT OSPRAY_ENABLE_APPS_TESTING)
  return()
endif()

add_all_subdirectories()
