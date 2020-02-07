## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# Define an option to enable/disable this module.
#
# Though not actually required we guard each module 'moduleName' with
# a OSPRAY_MODULE_MODULENAME flag that enables/disables its building.
#
option(OSPRAY_MODULE_BILINEAR_PATCH "Build pluggable geometry example module  (a Bilinear Patch)" OFF)

if (NOT OSPRAY_MODULE_BILINEAR_PATCH)
  return()
endif()

# build the actual new ospray geometry type.  since this plug in
# directly into the ospray core, we'll put it into a
# '<module>/ospray' subdirectory.
#
# This subdirectory will eventually build the
# libospray_module_<moduleName>.so library that defiens the actual
# core geometry type.
#
add_subdirectory(ospray)
