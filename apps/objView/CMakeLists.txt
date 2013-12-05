INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)

# --------------------------------------------
# main executable
# --------------------------------------------
ADD_EXECUTABLE(objView 
	# CPP sources
	objView.cpp
	)
TARGET_LINK_LIBRARIES(objView
	OSPRay
  ${OPENGL_LIBRARY} 
  ${GLUT_LIBRARY}
)
