## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (NOT OSPRAY_APPS_TESTING)
  return()
endif()

add_executable(ospConvertRawToAMR ospConvertRawToAMR.cpp)
target_link_libraries(ospConvertRawToAMR PRIVATE raw_to_amr)
install(TARGETS ospConvertRawToAMR
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
