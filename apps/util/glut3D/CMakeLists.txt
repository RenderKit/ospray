#INCLUDE(../../../cmake/glut.cmake)
ADD_LIBRARY(ospray_glut3d SHARED glut3D.cpp)
TARGET_LINK_LIBRARIES(ospray_glut3d ospray ${GLUT_LIBRARIES})


