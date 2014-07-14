# --------------------------------------------
# include dependencies (glut, glut3dwidget, ...)
# --------------------------------------------
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

ADD_SUBDIRECTORY(miniSG)

# --------------------------------------------
# main executable
# --------------------------------------------
ADD_EXECUTABLE(ospModelViewer
  # CPP sources
  msgView.cpp
  )
TARGET_LINK_LIBRARIES(ospModelViewer
  ospray_minisg
  ospray_glut3d
  ospray
  ${OPENGL_LIBRARY} 
  ${GLUT_LIBRARY}
  )
# ------------------------------------------------------------
INSTALL(TARGETS ospModelViewer DESTINATION bin)
