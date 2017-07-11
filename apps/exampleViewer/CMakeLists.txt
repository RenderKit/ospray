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

ospray_create_application(ospExampleViewer
  ospExampleViewer.cpp
  LINK
  ${OPENGL_LIBRARIES}
  ${GLFW_LIBRARY}
  gl3w
  imgui
  ospray
  ospray_common
  ospray_imgui3d_sg
  ospray_sg
)
