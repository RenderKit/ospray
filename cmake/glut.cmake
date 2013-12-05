# --------------------------------------------
# GL/GLUT stuff
# --------------------------------------------
IF (APPLE)
	FIND_PACKAGE(GLUT REQUIRED)
	FIND_PACKAGE(OpenGL REQUIRED)
ELSE(APPLE)
	FIND_PATH(GLUT_INCLUDE_DIR GL/glut.h 
		/usr/include # default
		/opt/apps/freeglut/2.6.0/include/ # TACC
		)
  SET (OPENGL_LIBRARY GL)
	INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIR})
	FIND_PATH(GLUT_LINK_DIR libglut.so
		/usr/lib64 /usr/lib # default
		/opt/apps/freeglut/2.6.0/lib/ # TACC
		)
	LINK_DIRECTORIES(${GLUT_LINK_DIR})
	FIND_PACKAGE(GLUT REQUIRED)
	SET(GLUT_LIBRARY glut GLU m)
ENDIF(APPLE)
