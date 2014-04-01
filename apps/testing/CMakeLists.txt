CONFIGURE_OSPRAY()

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)
INCLUDE (${EMBREE_DIR}/common/cmake/ispc.cmake)
INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

# ------------------------------------------------------------------
# test01: just opens a window, does nothing
# ------------------------------------------------------------------
#ADD_EXECUTABLE(test01 test01.cpp)
#TARGET_LINK_LIBRARIES(test01 osp_glut3d)

# ------------------------------------------------------------------
# test02: opens a window, renders a simple test frame
# ------------------------------------------------------------------
#ADD_EXECUTABLE(test02 test02.cpp)
#TARGET_LINK_LIBRARIES(test02 osp_glut3d)

# ------------------------------------------------------------------
# test03: same as test02, just uses ISPC for rendering that frame
# ------------------------------------------------------------------
#ISPC_COMPILE(${OSPRAY_ISPC_TARGET} test03.ispc)
#ADD_EXECUTABLE(test03 test03.cpp test03.dev${OSPRAY_ISPC_SUFFIX})
#TARGET_LINK_LIBRARIES(test03 osp_glut3d)

# ------------------------------------------------------------------
# test04: opens a window, renders a simple test frame
# ------------------------------------------------------------------
#ADD_EXECUTABLE(test04 test04.cpp)
#TARGET_LINK_LIBRARIES(test04 osp_glut3d osp_api)

# ------------------------------------------------------------------
# test05: opens a window, renders (via raycast) a test arrow widget
# ------------------------------------------------------------------
#ADD_EXECUTABLE(test05 test05.cpp)
#TARGET_LINK_LIBRARIES(test05 osp_glut3d osp_api)

IF (NOT THIS_IS_MIC)
	ADD_EXECUTABLE(streamLineView streamLineView.cpp)
	TARGET_LINK_LIBRARIES(streamLineView ospray_glut3d ospray)
ENDIF()