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
  "file:///opt/ospray/test-data/%(algo)/%(hash)"
  "file:///opt/ospray/benchmark-data/%(algo)/%(hash)"
  "file://N:/OSPRay/test-data/%(algo)/%(hash)"
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

