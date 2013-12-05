INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)

# --------------------------------------------
# main executable
# --------------------------------------------
ADD_EXECUTABLE(test01 
	# CPP sources
	test01.cpp
	)
TARGET_LINK_LIBRARIES(test01
	OSPglut3D
)
