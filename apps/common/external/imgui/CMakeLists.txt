## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.14)

include(FetchContent)
set(FETCHCONTENT_QUIET ON)
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG ${OSPRAY_IMGUI_GIT_TAG}
  # `patch` is not available on all systems, so use `git apply` instead.
  # The patch command first resets the repository to a clean state. That
  # is because CMake wants to apply the patch again when CMake needs to
  # reconfigure, which fails.
  PATCH_COMMAND git reset --hard && git apply -v -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/imgui.patch
)

FetchContent_MakeAvailable(imgui)

mark_as_advanced(FETCHCONTENT_BASE_DIR)
mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
mark_as_advanced(FETCHCONTENT_QUIET)
mark_as_advanced(FETCHCONTENT_SOURCE_DIR_IMGUI)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_IMGUI)

ospray_disable_compiler_warnings()

add_library(ospray_imgui SHARED
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)

target_link_libraries(ospray_imgui
PUBLIC
  glfw
  ${OPENGL_LIBRARIES}
)

ospray_install_library(ospray_imgui apps)

target_include_directories(
  ospray_imgui
INTERFACE
  $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
  $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/backends>
PRIVATE
  ${imgui_SOURCE_DIR}
)
