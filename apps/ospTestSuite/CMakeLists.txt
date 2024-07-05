## Copyright 2017 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (NOT OSPRAY_ENABLE_APPS_TESTING)
  return()
endif()

# Note: CMake provides FindGTest which defines target GTest::GTest
find_package(GTest REQUIRED CONFIG)

ospray_disable_compiler_warnings()

add_library(ospray_gtest_utils
  environment.cpp
  test_tools.cpp
  test_fixture.cpp
)

target_link_libraries(ospray_gtest_utils
PUBLIC
  arcball_camera
  ospray_testing
  GTest::gtest
  stb_image
)

target_include_directories(ospray_gtest_utils
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)

add_executable(ospTestSuite
  ${OSPRAY_RESOURCE}
  environment.cpp # potentially compile with SYCL, overriding non-SYCL ospray_gtest_utils
  test_geometry.cpp
  test_volumetric.cpp
  test_appearance.cpp
  test_assigned_data.cpp
  test_sharedusm_data.cpp
  test_light.cpp
  test_enums.cpp
  $<$<BOOL:${OSPRAY_APPS_ENABLE_GLM}>:test_glm_compat.cpp>
  test_camera.cpp
  test_motionblur.cpp
  test_framebuffer.cpp
  test_interpolation.cpp
  test_imageop.cpp
  ospTestSuite.cpp
)

# We need to know if volumes are enabled at compile time so that
# we can enable/disable the VKL-dependent tests for enum values
# Same for OIDN tests
target_compile_definitions(ospTestSuite
PRIVATE
  $<$<BOOL:${OSPRAY_ENABLE_VOLUMES}>:OSPRAY_ENABLE_VOLUMES>
  $<$<BOOL:${OSPRAY_MODULE_DENOISER}>:OSPRAY_MODULE_DENOISER>
)
target_include_directories(ospTestSuite
PRIVATE
  $<$<BOOL:${OSPRAY_ENABLE_VOLUMES}>:$<TARGET_PROPERTY:openvkl::openvkl,INTERFACE_INCLUDE_DIRECTORIES>>
  $<$<BOOL:${OSPRAY_MODULE_DENOISER}>:$<TARGET_PROPERTY:OpenImageDenoise,INTERFACE_INCLUDE_DIRECTORIES>>
)

target_link_libraries(ospTestSuite
PRIVATE
  ospray_testing
  ospray_gtest_utils
  embree
  $<$<BOOL:${OSPRAY_APPS_ENABLE_GLM}>:${GLM_TARGET}>
)
if (OSPRAY_MODULE_GPU)
  ospray_add_sycl_target(ospTestSuite)
endif()

ospray_sign_target(ospTestSuite)

install(TARGETS ospTestSuite
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
