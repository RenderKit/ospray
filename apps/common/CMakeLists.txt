## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (NOT OSPRAY_APPS_BENCHARK AND
    NOT OSPRAY_APPS_EXAMPLES AND
    NOT OSPRAY_APPS_TESTING)
  return()
endif()

add_all_subdirectories()
