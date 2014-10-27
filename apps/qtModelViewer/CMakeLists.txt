# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/)

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
  # qt helper widgets
  widgets/affineSpaceManipulator/HelperGeometry.cpp
  widgets/affineSpaceManipulator/QAffineSpaceManipulator.cpp
  widgets/transferFunction/QTransferFunctionEditor.cpp

  # modelviewer proper
  ModelViewer.cpp

  # scene graph and importers
#  sg/ImportRIVL
 # sg/Scene
 sg/SceneGraph.cpp
 sg/ImportSpheres.cpp
 sg/Renderer.cpp
sg/ImportOSP.cpp
  # the (th)ing to launch them all, and in darkness bind them...
  main.cpp
  )

SET(MOC_HEADERS
  ModelViewer.h
  widgets/transferFunction/QTransferFunctionEditor.h
  widgets/affineSpaceManipulator/QAffineSpaceManipulator.h
  )

QT4_WRAP_CPP(MOC_OUTFILES ${MOC_HEADERS})

ADD_EXECUTABLE(ospQTV ${SRCS} ${MOC_OUTFILES})
TARGET_LINK_LIBRARIES(ospQTV ${LIBS} ospray_xml${OSPRAY_LIB_SUFFIX})
# ------------------------------------------------------------
INSTALL(TARGETS ospQTV DESTINATION bin)
# ------------------------------------------------------------

