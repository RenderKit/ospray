# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

# setup dependencies
SET(LIBS ${LIBS} ospray ospray_fileio)

FIND_PACKAGE(Qt4 REQUIRED) 
SET(QT_USE_QTOPENGL TRUE)
INCLUDE(${QT_USE_FILE})
SET(LIBS ${LIBS} ${QT_LIBRARIES})

FIND_PACKAGE(OpenGL REQUIRED)
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${OPENGL_LIBRARIES})

SET(SRCS ${SRCS}
  main.cpp
  ColorMap.cpp
  QOSPRayWindow.cpp
  SliceWidget.cpp
  TransferFunctionEditor.cpp
  TransferFunctionPiecewiseLinearWidget.cpp
  VolumeViewer.cpp
  )

SET(MOC_HEADERS
  SliceWidget.h
  TransferFunctionEditor.h
  TransferFunctionPiecewiseLinearWidget.h
  VolumeViewer.h
  )

QT4_WRAP_CPP(MOC_OUTFILES ${MOC_HEADERS})

ADD_EXECUTABLE(ospVolumeViewer ${SRCS} ${MOC_OUTFILES})
TARGET_LINK_LIBRARIES(ospVolumeViewer ${LIBS})
# ------------------------------------------------------------
INSTALL(TARGETS ospVolumeViewer DESTINATION bin)
# ------------------------------------------------------------

