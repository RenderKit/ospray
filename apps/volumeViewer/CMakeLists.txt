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
  LightEditor.cpp
  PLYGeometryFile.cpp
  QOSPRayWindow.cpp
  SliceWidget.cpp
  TransferFunctionEditor.cpp
  TransferFunctionPiecewiseLinearWidget.cpp
  VolumeViewer.cpp
  )

SET(MOC_HEADERS
  LightEditor.h
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

