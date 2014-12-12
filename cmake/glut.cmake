## ======================================================================== ##
## Copyright 2009-2014 Intel Corporation                                    ##
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

IF (APPLE)
  FIND_PACKAGE(GLUT REQUIRED)
  FIND_PACKAGE(OpenGL REQUIRED)
  INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIR})
ELSE(APPLE)
  SET (OPENGL_LIBRARY GL)
  FIND_PACKAGE(GLUT)
  IF (GLUT_FOUND)
    SET(GLUT_LIBRARY glut GLU m)
  ELSE()
    FIND_PATH(GLUT_INCLUDE_DIR GL/glut.h 
      $ENV{TACC_FREEGLUT_INC}
      )
    FIND_PATH(GLUT_LINK_DIR libglut.so
      $ENV{TACC_FREEGLUT_LIB}
      )
    FIND_LIBRARY(GLUT_LIBRARY NAMES libglut.so PATHS $ENV{TACC_FREEGLUT_LIB})
    IF (${GLUT_LIBRARY} STREQUAL "GLUT_LIBRARY-NOTFOUND")
      MESSAGE("-- Could not find GLUT library, even after trying additional search dirs")	
    ELSE()
      SET(GLUT_FOUND ON)
    ENDIF()
  ENDIF()
ENDIF(APPLE)

INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIR})
LINK_DIRECTORIES(${GLUT_LINK_DIR})
SET(GLUT_LIBRARIES ${GLUT_LIBRARY} ${OPENGL_LIBRARY})
