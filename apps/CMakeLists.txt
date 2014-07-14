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
  ADD_SUBDIRECTORY(modelViewer)

  # stream line viewer for NASA Stream Line Demo
  ADD_SUBDIRECTORY(streamLineViewer)

  # volume viewer application
  OPTION(BUILD_VOLUMEVIEWER "Build volume viewer application" OFF)
  IF(BUILD_VOLUMEVIEWER)
    ADD_SUBDIRECTORY(volumeViewer)
  ENDIF()
ENDIF()
