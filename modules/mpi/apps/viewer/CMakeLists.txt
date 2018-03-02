## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
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

include_directories(${CMAKE_SOURCE_DIR}/apps/common
                    ${CMAKE_SOURCE_DIR}/apps/exampleViewer
                    ${GLFW_INCLUDE_DIRS})

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
  gensv
  ${GLFW_LIBRARY}
)

