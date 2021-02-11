## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

ospray_disable_compiler_warnings()

add_library(ospray_imgui SHARED
  imgui.cpp
  imgui_draw.cpp
  imgui_tables.cpp
  imgui_widgets.cpp
)

target_link_libraries(ospray_imgui
PUBLIC
  ${OPENGL_LIBRARIES}
  rkcommon::rkcommon
)

ospray_install_library(ospray_imgui apps)

target_include_directories(
  ospray_imgui
INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)
