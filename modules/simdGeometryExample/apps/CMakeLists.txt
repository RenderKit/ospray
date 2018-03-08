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

# this subdirectory demonstrates on how to _use_ demo module from
# within a simple viewer

# the actual example viewer. Note this viewer does _NOT_ link to the
# actual module library - the module should (and _does_, in this
# example) get loaded through ospLoadModule(...), which will make sure
# the module gets properly oaded on all worker nodes, even if those
# live on different nodes, accelerator cards, memory spaces, etc.
ospray_create_application(ospSimdBilinearPatchViewer
  # the actual viewer
  bilinearPatchViewer.cpp
  # helper code to parse demo files
  Patch.cpp

  LINK

  ospray
  ospray_common
  ospray_imgui3d_sg
  ospray_sg
  ${OPENGL_LIBRARIES}
  ${GLUT_LIBRARIES}
)
