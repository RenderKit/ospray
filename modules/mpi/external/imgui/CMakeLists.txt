## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(imgui STATIC
  imgui.cpp
  imgui_draw.cpp
  imgui_demo.cpp
)

target_link_libraries(imgui
PUBLIC
  rkcommon::rkcommon
)

target_include_directories(
  imgui
INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)
