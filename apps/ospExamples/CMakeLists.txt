## Copyright 2018-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (NOT OSPRAY_APPS_EXAMPLES)
  return()
endif()

# prefer libGL over libOpenGl for better compatibility with SWR
set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL 2 REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(ospExamples
  ${OSPRAY_RESOURCE}
  GLFWOSPRayWindow.cpp
  imgui_impl_glfw_gl3.cpp
  ospExample.cpp
)

target_link_libraries(ospExamples
PRIVATE
  arcball_camera
  ospray_testing
  ospray_imgui
  glfw
  ${OPENGL_LIBRARIES}
)

install(TARGETS ospExamples
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
