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
OSPRAY_BUILD_COMPONENT(ospcommon/testing/gtest)

MARK_AS_ADVANCED(gtest_build_samples)
MARK_AS_ADVANCED(gtest_build_tests)
MARK_AS_ADVANCED(gtest_disable_pthreads)
MARK_AS_ADVANCED(gtest_force_shared_crt)
MARK_AS_ADVANCED(gtest_hide_internal_symbols)

##############################################################
# Tests
##############################################################
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/ospray/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/tests/include)

SET(TESTS_SOURCES
	sources/ospray_environment.cpp
	sources/ospray_test_fixture.cpp
	sources/ospray_test_geometry.cpp
	sources/ospray_test_volumetric.cpp
	sources/ospray_test_tools.cpp
	)

ADD_EXECUTABLE(ospray_test_suite ospray_test_suite.cpp ${TESTS_SOURCES})
SET_TARGET_PROPERTIES(ospray_test_suite PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/regression_tests)

TARGET_LINK_LIBRARIES(ospray_test_suite gtest_main)
TARGET_LINK_LIBRARIES(ospray_test_suite ospray)

# Baseline images for tests and benchmarks
include(ExternalData)

set(ExternalData_URL_TEMPLATES
	"http://sdvis.org/ospray/download/baseline/test-data/%(algo)/%(hash)"
	"http://sdvis.org/ospray/download/baseline/benchmark-data/%(algo)/%(hash)"
	)

set(ExternalData_OBJECT_STORES
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

