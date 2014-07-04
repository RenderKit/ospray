# --------------------------------------------
# include dependencies (glut, glut3dwidget, ...)
# --------------------------------------------
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

# --------------------------------------------
# main executable
# --------------------------------------------
ADD_EXECUTABLE(msgView 
	# CPP sources
	msgView.cpp
	)
TARGET_LINK_LIBRARIES(msgView
	ospray_minisg
	ospray_glut3d
	ospray
  ${OPENGL_LIBRARY} 
  ${GLUT_LIBRARY}
)
