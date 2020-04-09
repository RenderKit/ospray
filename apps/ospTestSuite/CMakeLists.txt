## Copyright 2017-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (NOT OSPRAY_APPS_TESTING)
  return()
endif()

ospray_disable_compiler_warnings()

add_executable(ospTestSuite
  ${OSPRAY_RESOURCE}
  environment.cpp
  test_fixture.cpp
  test_tools.cpp
  test_geometry.cpp
  test_volumetric.cpp
  test_appearance.cpp
  test_light.cpp
  test_enums.cpp
  ospTestSuite.cpp
)

target_link_libraries(ospTestSuite
PRIVATE
  arcball_camera
  ospray_testing
  ospray_module_ispc
  gtest_main
  stb_image
)

install(TARGETS ospTestSuite
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT test
)
