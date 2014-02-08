# --------------------------------------------
# include dependencies (glut, glut3dwidget, ...)
# --------------------------------------------
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

# --------------------------------------------
# main executable
# --------------------------------------------
ADD_EXECUTABLE(ospDVR 
	# CPP sources
	ospDVR.cpp
	)
TARGET_LINK_LIBRARIES(ospDVR
	osp_glut3d
	ospray
  ${OPENGL_LIBRARY} 
  ${GLUT_LIBRARY}
)
