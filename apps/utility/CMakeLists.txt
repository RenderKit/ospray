## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (NOT OSPRAY_APPS_TESTING)
  return()
endif()

add_executable(ospConvertRawToAMR ${OSPRAY_RESOURCE} ospConvertRawToAMR.cpp)
target_link_libraries(ospConvertRawToAMR PRIVATE raw_to_amr ospray)
install(TARGETS ospConvertRawToAMR
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
