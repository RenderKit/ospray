## Copyright 2018 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (NOT OSPRAY_ENABLE_APPS_EXAMPLES)
  return()
endif()

add_executable(ospExamples
  ${OSPRAY_RESOURCE}
  GLFWOSPRayWindow.cpp
  ospExample.cpp
)

target_link_libraries(ospExamples
PRIVATE
  arcball_camera
  ospray_testing
  ospray_imgui
  ${OPENGL_LIBRARIES}
)

ospray_sign_target(ospExamples)

install(TARGETS ospExamples
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
