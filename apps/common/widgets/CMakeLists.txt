## ======================================================================== ##
## Copyright 2009-2016 Intel Corporation                                    ##
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

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

ADD_LIBRARY(ospray_glut3d SHARED glut3D.cpp)

TARGET_LINK_LIBRARIES(ospray_glut3d ospray
  ${OPENGL_LIBRARIES}
  ${GLUT_LIBRARIES}
)
OSPRAY_SET_LIBRARY_VERSION(ospray_glut3d)
INSTALL(TARGETS ospray_glut3d
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT lib
)
