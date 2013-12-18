# --------------------------------------------
# include dependencies (glut, glut3dwidget, ...)
# --------------------------------------------
INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

# --------------------------------------------
# main executable
# --------------------------------------------
ADD_EXECUTABLE(msgView 
	# CPP sources
	msgView.cpp
	)
TARGET_LINK_LIBRARIES(msgView
	OSPRay
	OSPMiniSG
  ${OPENGL_LIBRARY} 
  ${GLUT_LIBRARY}
)
