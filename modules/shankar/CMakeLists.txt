SET(MODULE_SHANKARVIEW OFF CACHE BOOL "Build 'TACH' (shankarView model Viewer)")

IF (MODULE_SHANKARVIEW)

		CONFIGURE_OSPRAY()

		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
		INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/apps/util)
		INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
		INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
		INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

		OSPRAY_ADD_LIBRARY(ospray_module_shankar${OSPRAY_LIB_SUFFIX} SHARED
			disks.ispc
			disks.cpp
			rbfTree.ispc
			rbfTree.cpp
			)

		IF (NOT THIS_IS_MIC)
			ADD_EXECUTABLE(ospShankarViewer 
				shankarViewer.cpp
				shankarModel.cpp
				)
			TARGET_LINK_LIBRARIES(ospShankarViewer 
				ospray_module_shankar${OSPRAY_LIB_SUFFIX}
				ospray ospray_glut3d)
		ENDIF()
#	ENDIF()
ENDIF()
