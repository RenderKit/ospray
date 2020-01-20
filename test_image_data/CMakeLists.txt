## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.1)

project(ospray_test_data)

# Baseline images for tests and benchmarks
include(ExternalData)

set(ExternalData_URL_TEMPLATES
  "http://sdvis.org/ospray/download/baseline/test-data/%(algo)/%(hash)"
  "http://sdvis.org/ospray/download/baseline/benchmark-data/%(algo)/%(hash)"
)

set(ExternalData_OBJECT_STORES
  "${CMAKE_BINARY_DIR}/regression_test_baseline"
  "/opt/ospray/test-data"
  "//sdvis-nas/NAS/OSPRay/test-data"
)

set(ExternalData_BINARY_ROOT ${CMAKE_BINARY_DIR}/regression_test_baseline)
set(ExternalData_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/baseline)

ExternalData_Expand_Arguments(ospray_test_data
  ospray_test_suite
  DATA{${ExternalData_SOURCE_ROOT}/,REGEX:.*}
)

ExternalData_Add_Target(ospray_test_data)

set_target_properties(ospray_test_data PROPERTIES EXCLUDE_FROM_ALL TRUE)
