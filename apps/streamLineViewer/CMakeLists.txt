CONFIGURE_OSPRAY()

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)
INCLUDE (${EMBREE_DIR}/common/cmake/ispc.cmake)
INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

IF (NOT THIS_IS_MIC)
  ADD_EXECUTABLE(ospStreamLineViewer streamLineView.cpp)
  TARGET_LINK_LIBRARIES(ospStreamLineViewer ospray_glut3d ospray)
  # ------------------------------------------------------------
  INSTALL(TARGETS ospStreamLineViewer DESTINATION bin)
  # ------------------------------------------------------------
ENDIF()