CONFIGURE_OSPRAY()

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/apps/common)

ADD_SUBDIRECTORY(miniSG)

# --------------------------------------------
# main executable
# --------------------------------------------
ADD_EXECUTABLE(ospModelViewer
  # CPP sources
  modelViewer.cpp
  )
TARGET_LINK_LIBRARIES(ospModelViewer
  ospray_minisg
  ospray_glut3d
  ${OPENGL_LIBRARY} 
  ${GLUT_LIBRARY}
  )
# ------------------------------------------------------------
INSTALL(TARGETS ospModelViewer DESTINATION bin)
