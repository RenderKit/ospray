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

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/../common/gl3w
  ${CMAKE_CURRENT_LIST_DIR}/../common/imgui
  ${GLFW_INCLUDE_DIRS}
)

ospray_create_library(ospray_imgui3d_sg
  Imgui3dExport.h
  imgui_impl_glfw_gl3.cpp
  imgui3D.cpp
  imguiViewer.cpp
  transferFunction.cpp
  cameraManipulator.cpp
LINK
  ospray
  ospray_common
  ospray_imgui
  ospray_imgui_util_sg
  ospray_tfn
  ${GLFW_LIBRARY}
)
