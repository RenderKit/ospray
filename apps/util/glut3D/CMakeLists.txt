INCLUDE(../../../cmake/glut.cmake)
ADD_LIBRARY(osp_glut3d SHARED glut3D.cpp)
TARGET_LINK_LIBRARIES(osp_glut3d osp_common ${GLUT_LIBRARIES})


