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

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(LIBS ${LIBS} ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES})

FIND_PACKAGE(Qt4 REQUIRED) 
SET(QT_USE_QTOPENGL TRUE)
FIND_PACKAGE(OpenGL REQUIRED)
INCLUDE(${QT_USE_FILE})
SET(LIBS ${LIBS} ${QT_LIBRARIES} ${OPENGL_LIBRARIES})

ADD_SUBDIRECTORY(sg)

SET(SRCS ${SRCS}
  # qt helper widgets
  widgets/affineSpaceManipulator/HelperGeometry.cpp
  widgets/affineSpaceManipulator/QAffineSpaceManipulator.cpp
  widgets/transferFunction/QTransferFunctionEditor.cpp
  widgets/lightManipulator/QLightManipulator.cpp

  # modelviewer proper
  ModelViewer.cpp
  FPSCounter.cpp

  # the (th)ing to launch them all, and in darkness bind them...
  main.cpp
  )

SET(MOC_HEADERS
  ModelViewer.h
  widgets/transferFunction/QTransferFunctionEditor.h
  widgets/affineSpaceManipulator/QAffineSpaceManipulator.h
  widgets/lightManipulator/QLightManipulator.h
  )

QT4_WRAP_CPP(MOC_OUTFILES ${MOC_HEADERS})

OSPRAY_CREATE_APPLICATION(QtViewer
  ${SRCS}
  ${MOC_OUTFILES}
LINK
  ${LIBS} 
  ospray_sg
  ${TBB_LIBRARY_MALLOC}
  ${TBB_LIBRARY}
)
