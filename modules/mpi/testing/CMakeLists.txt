## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

ospray_disable_compiler_warnings()

add_executable(ospMPIDistribTestSuite
  ${OSPRAY_RESOURCE}
  distributed_test_fixture.cpp
  ospMPIDistribTestSuite.cpp
)

target_link_libraries(ospMPIDistribTestSuite
PRIVATE
  arcball_camera
  ospray_testing
  ospray_module_cpu
  ospray_gtest_utils
  MPI::MPI_CXX
)

ospray_sign_target(ospMPIDistribTestSuite)

install(TARGETS ospMPIDistribTestSuite
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
