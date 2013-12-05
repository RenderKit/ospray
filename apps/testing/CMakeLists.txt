CONFIGURE_OSPRAY()

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)
INCLUDE (${EMBREE_DIR}/common/cmake/ispc.cmake)

# ------------------------------------------------------------------
# test01: just opens a window, does nothing
# ------------------------------------------------------------------
ADD_EXECUTABLE(test01 test01.cpp)
TARGET_LINK_LIBRARIES(test01 OSPglut3D)

# ------------------------------------------------------------------
# test02: opens a window, renders a simple test frame
# ------------------------------------------------------------------
ADD_EXECUTABLE(test02 test02.cpp)
TARGET_LINK_LIBRARIES(test02 OSPglut3D)

# ------------------------------------------------------------------
# test03: same as test02, just uses ISPC for rendering that frame
# ------------------------------------------------------------------
ISPC_COMPILE(${OSPRAY_TARGET} test03.ispc)
ADD_EXECUTABLE(test03 test03.cpp test03.dev${OSPRAY_ISPC_SUFFIX})
TARGET_LINK_LIBRARIES(test03 OSPglut3D)

