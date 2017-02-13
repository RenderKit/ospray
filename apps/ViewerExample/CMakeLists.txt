## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
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

OPTION(OSPRAY_APPS_VIEWEREXAMPLE "Build 'Viewer' example app" OFF)

if (OSPRAY_APPS_VIEWEREXAMPLE)
  if (NOT OSPRAY_APPS_SG)
    message(ERROR "OSPRAY_APPS_SG required to build OSPRAY_APPS_VIEWEREXAMPLE")
  endif()
  find_package(OpenGL REQUIRED)

  ##############################################################
  # Build support libs
  ##############################################################

  add_subdirectory(common)
  add_subdirectory(widgets)

  ##############################################################
  # Build app
  ##############################################################

  include_directories(
    ${CMAKE_SOURCE_DIR}
    common/gl3w
    common/imgui
    ${OPENGL_INCLUDE_DIR}
  )

ospray_create_application(ospViewerExample
    ospImGui.cpp
    LINK
    ${OPENGL_LIBRARIES}
    glfw
    gl3w
    imgui
    ospray
    ospray_common
    ospray_commandline
    ospray_imgui3d
    ospray_sg
  )

endif()
