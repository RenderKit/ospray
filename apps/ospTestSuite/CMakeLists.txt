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

if (OSPRAY_ENABLE_VOLUMES)
  # We need to know if volumes are enabled at compile time so that
  # we can enable/disable the VKL-dependent tests for enum values
  target_compile_definitions(ospray_gtest_utils
    PUBLIC
    -DOSPRAY_ENABLE_VOLUMES
  )
endif()

add_executable(ospTestSuite
  ${OSPRAY_RESOURCE}
  test_geometry.cpp
  test_volumetric.cpp
  test_appearance.cpp
  test_light.cpp
  test_enums.cpp
  $<$<BOOL:${OSPRAY_APPS_ENABLE_GLM}>:test_glm_compat.cpp>
  test_camera.cpp
  test_motionblur.cpp
  test_framebuffer.cpp
  test_interpolation.cpp
  ospTestSuite.cpp
)

target_link_libraries(ospTestSuite
PRIVATE
  ospray_testing
  ospray_gtest_utils
  embree
  $<$<BOOL:${OSPRAY_ENABLE_VOLUMES}>:openvkl::openvkl>
  $<$<BOOL:${OSPRAY_APPS_ENABLE_GLM}>:${GLM_TARGET}>
)

ospray_sign_target(ospTestSuite)

install(TARGETS ospTestSuite
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
