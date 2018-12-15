## ======================================================================== ##
## Copyright 2017-2018 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

ospray_disable_compiler_warnings()

##############################################################
# Google Test
##############################################################
ospray_build_component(ospcommon/testing/gtest)

mark_as_advanced(gtest_build_samples)
mark_as_advanced(gtest_build_tests)
mark_as_advanced(gtest_disable_pthreads)
mark_as_advanced(gtest_force_shared_crt)
mark_as_advanced(gtest_hide_internal_symbols)

##############################################################
# Tests
##############################################################
include_directories(${CMAKE_SOURCE_DIR}/ospray/include)
include_directories(${CMAKE_SOURCE_DIR}/components)
include_directories(${CMAKE_SOURCE_DIR}/tests/include)

set(TESTS_SOURCES
	sources/ospray_environment.cpp
	sources/ospray_test_fixture.cpp
	sources/ospray_test_geometry.cpp
	sources/ospray_test_volumetric.cpp
	sources/ospray_test_tools.cpp
	)

ospray_create_test(ospray_test_suite
  ospray_test_suite.cpp
  ${TESTS_SOURCES}
LINK
  gtest_main
  ospray
  ospray_common
)
set_target_properties(ospray_test_suite PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/regression_tests)

# Baseline images for tests and benchmarks
include(ExternalData)

set(ExternalData_URL_TEMPLATES
	"http://sdvis.org/ospray/download/baseline/test-data/%(algo)/%(hash)"
	"http://sdvis.org/ospray/download/baseline/benchmark-data/%(algo)/%(hash)"
	)

set(ExternalData_OBJECT_STORES
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/regression_tests/baseline"
	"/opt/ospray/test-data"
	"//sdvis-nas/NAS/OSPRay/test-data"
	)
set(ExternalData_BINARY_ROOT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/regression_tests/baseline)
set(ExternalData_SOURCE_ROOT ${CMAKE_SOURCE_DIR}/tests/baseline)

ExternalData_Expand_Arguments(ospray_test_data
	ospray_test_suite
	DATA{${ExternalData_SOURCE_ROOT}/,REGEX:.*}
	)
ExternalData_Add_Target(ospray_test_data)
set_target_properties(ospray_test_data PROPERTIES EXCLUDE_FROM_ALL TRUE)

set(ExternalData_BINARY_ROOT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/benchmarks/baseline)
set(ExternalData_SOURCE_ROOT ${CMAKE_SOURCE_DIR}/scripts/bench/baseline)

ExternalData_Expand_Arguments(ospray_bench_data
	ospray_test_suite
	DATA{${ExternalData_SOURCE_ROOT}/,REGEX:.*}
	)
ExternalData_Add_Target(ospray_bench_data)
set_target_properties(ospray_bench_data PROPERTIES EXCLUDE_FROM_ALL TRUE)

if (OSPRAY_AUTO_DOWNLOAD_TEST_IMAGES)
  add_dependencies(ospray_test_suite ospray_test_data)
endif()

