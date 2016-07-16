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

IF (OSPRAY_MODULE_DISPLAY_WALD)
	INCLUDE_DIRECTORIES(${DISPLAY_WALD_INCLUDE_DIR})
	ADD_DEFINITIONS(-DOSPRAY_DISPLAY_WALD=1)
ENDIF()


SET(LIBS
	ospray
  ospray_common
  ${OPENGL_LIBRARIES}
  ${GLUT_LIBRARIES}
	)

OSPRAY_CREATE_LIBRARY(glut3d
  Glut3dExport.h
  glut3D.cpp
  OSPGlutViewer.cpp
	LINK
	${LIBS}
)
