## ======================================================================== ##
## Copyright 2009-2015 Intel Corporation                                    ##
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

CONFIGURE_OSPRAY()

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)

INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

IF (NOT THIS_IS_MIC)
  ADD_EXECUTABLE(ospStreamLineViewer StreamLineViewer.cpp)
  TARGET_LINK_LIBRARIES(ospStreamLineViewer
    ospray_glut3d
    ospray
    ospray_xml
    ${TBB_LIBRARY_MALLOC}
    ${TBB_LIBRARY}
  )
  # ------------------------------------------------------------
  INSTALL(TARGETS ospStreamLineViewer DESTINATION bin)
  # ------------------------------------------------------------
ENDIF()
