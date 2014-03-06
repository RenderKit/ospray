INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)

#INCLUDE(${EMBREE_DIR}/common/cmake/ispc.cmake)
INCLUDE_DIRECTORIES(${EMBREE_DIR})
INCLUDE_DIRECTORIES(${EMBREE_DIR}/common)
INCLUDE_DIRECTORIES(${EMBREE_DIR}/kernels)
INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR} ${PROJECT_SOURCE_DIR})

CONFIGURE_OSPRAY()

IF(NOT THIS_IS_MIC)
# common utility classes for 3D viewer widget, importers, etc
ADD_SUBDIRECTORY(util)
# mini-scene graph viewer for _geometry_ (intentionally simple)
ADD_SUBDIRECTORY(msgView)
# mini dvr volume viewer (intentionally simple)
ADD_SUBDIRECTORY(ospDVR)
ENDIF()

# general testing codes; to be removed at some point in time
#ADD_SUBDIRECTORY(testing)
# mpi testing codes; to be removed at some point in time
ADD_SUBDIRECTORY(mpi-testing)
