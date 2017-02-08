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

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

ADD_SUBDIRECTORY(glUtil)

# setup dependencies
SET(LIBS ${LIBS} ospray)
SET(LIBS ${LIBS} ospray_common)
SET(LIBS ${LIBS} ospray_importer)
SET(LIBS ${LIBS} ospray_opengl_util)
SET(LIBS ${LIBS} ospray_tfn)
SET(LIBS ${LIBS} ospray_minisg)
SET(LIBS ${LIBS} ospray_commandline)

# link optional loaders-based modules if built
IF (OSPRAY_MODULE_SEISMIC)
  SET(LIBS ${LIBS} ospray_module_seismic)
ENDIF (OSPRAY_MODULE_SEISMIC)

FIND_PACKAGE(Qt4 REQUIRED QtGui QtOpenGL)
SET(QT_USE_QTOPENGL TRUE)
INCLUDE(${QT_USE_FILE})
SET(LIBS ${LIBS} Qt4::QtGui Qt4::QtOpenGL)

FIND_PACKAGE(OpenGL REQUIRED)
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${OPENGL_LIBRARIES} ${TBB_LIBRARY_MALLOC} ${TBB_LIBRARY})

SET(SRCS ${SRCS}
  main.cpp
  ColorMap.cpp
  IsosurfaceEditor.cpp
  IsovalueWidget.cpp
  LightEditor.cpp
  LinearTransferFunctionWidget.cpp
  OpenGLAnnotationRenderer.cpp
  PreferencesDialog.cpp
  ProbeWidget.cpp
  QOSPRayWindow.cpp
  SliceEditor.cpp
  SliceWidget.cpp
  TransferFunctionEditor.cpp
  VolumeViewer.cpp
)

SET(MOC_HEADERS
  IsosurfaceEditor.h
  IsovalueWidget.h
  LightEditor.h
  LinearTransferFunctionWidget.h
  OpenGLAnnotationRenderer.h
  PreferencesDialog.h
  ProbeWidget.h
  QOSPRayWindow.h
  SliceEditor.h
  SliceWidget.h
  TransferFunctionEditor.h
  VolumeViewer.h
)

QT4_WRAP_CPP(MOC_OUTFILES ${MOC_HEADERS})

OSPRAY_CREATE_APPLICATION(ospVolumeViewer
  ${SRCS}
  ${MOC_OUTFILES}
LINK
  ${LIBS} 
)
