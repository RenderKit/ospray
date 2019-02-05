## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

ospray_disable_compiler_warnings()

find_package(glfw3 QUIET)
if (TARGET glfw)
  get_property(glfw3_INCLUDE_DIRS TARGET glfw PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
endif()

ospray_create_application(ospDistribViewerDemo
  ospDistribViewerDemo.cpp
  arcball.cpp
LINK
  ospray
  ospray_common
  ospray_mpi_common
  ospray_imgui_util_sg
  ospray_imgui3d_sg
  ospray_sg
  ospray_tfn
  ospray_gensv
)

target_include_directories(ospDistribViewerDemo
  PRIVATE
    ${CMAKE_SOURCE_DIR}/apps
    ${CMAKE_SOURCE_DIR}/apps/exampleViewer
    ${CMAKE_CURRENT_LIST_DIR}/..
)

target_compile_definitions(ospDistribViewerDemo PRIVATE
  -DSHOW_WINDOW=1)

