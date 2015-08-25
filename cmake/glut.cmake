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

FIND_PACKAGE(OpenGL REQUIRED)

IF (APPLE)
  FIND_PACKAGE(GLUT REQUIRED)
ELSEIF (WIN32)
# FindGLUT.cmake is broken for Windows in CMake until 3.0: does not support PATH
  IF (GLUT_ROOT_PATH)
    MESSAGE(WARNING "Warning: GLUT_ROOT_PATH is depreciated.")
    SET(DEPRECIATED_WIN32_INCLUDE ${GLUT_ROOT_PATH}/include)
    SET(DEPRECIATED_WIN32_RELEASE ${GLUT_ROOT_PATH}/Release)
  ENDIF()
  FIND_PATH(GLUT_INCLUDE_DIR
    NAMES GL/glut.h
    PATHS ${FREEGLUT_ROOT_PATH}/include
    ${PROJECT_SOURCE_DIR}/../freeglut/include
    ${PROJECT_SOURCE_DIR}/../freeglut-MSVC-3.0.0-2.mp/include ${PROJECT_SOURCE_DIR}/../freeglut-MSVC-3.0.0-2.mp/freeglut/include
    ${PROJECT_SOURCE_DIR}/../freeglut-MSVC-2.8.1-1.mp/include ${PROJECT_SOURCE_DIR}/../freeglut-MSVC-2.8.1-1.mp/freeglut/include
    ${DEPRECIATED_WIN32_INCLUDE}
  )
  # detect and select x64
  IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(ARCH x64)
  ENDIF()
  FIND_LIBRARY(GLUT_glut_LIBRARY
    NAMES freeglut glut glut32
    PATHS ${GLUT_INCLUDE_DIR}/../lib/${ARCH} ${FREEGLUT_ROOT_PATH}/lib/${ARCH} ${DEPRECIATED_WIN32_RELEASE}
  )
  SET(GLUT_LIBRARIES ${GLUT_glut_LIBRARY})
  MARK_AS_ADVANCED(
    GLUT_INCLUDE_DIR
    GLUT_glut_LIBRARY
  )
  IF (NOT GLUT_INCLUDE_DIR OR NOT GLUT_glut_LIBRARY)
    MESSAGE(FATAL_ERROR "Could not find GLUT library. You could fetch freeglut from http://www.transmissionzero.co.uk/software/freeglut-devel/ and set the FREEGLUT_ROOT_PATH variable in cmake.")
  ENDIF()
ELSE()
  FIND_PACKAGE(GLUT)
  IF (GLUT_FOUND)
    SET(GLUT_LIBRARIES glut GLU m)
  ELSE()
    FIND_PATH(GLUT_INCLUDE_DIR GL/glut.h 
      $ENV{TACC_FREEGLUT_INC}
      )
    FIND_PATH(GLUT_LINK_DIR libglut.so
      $ENV{TACC_FREEGLUT_LIB}
      )
    FIND_LIBRARY(GLUT_LIBRARIES NAMES libglut.so PATHS $ENV{TACC_FREEGLUT_LIB})
    IF (NOT GLUT_LIBRARIES)
      MESSAGE(FATAL_ERROR "Could not find GLUT library, even after trying additional search dirs")
    ELSE()
      SET(GLUT_FOUND ON)
    ENDIF()
  ENDIF()
ENDIF()

INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR} ${GLUT_INCLUDE_DIR})
