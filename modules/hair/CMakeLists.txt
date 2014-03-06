SET(MODULE_HAIR OFF CACHE BOOL "Build 'Hair' module")

IF (MODULE_HAIR)
	CONFIGURE_OSPRAY()
	if (NOT THIS_IS_MIC)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

		INCLUDE_DIRECTORIES(${EMBREE_DIR}/kernels/xeon/)

		ADD_LIBRARY(ospray_module_hair SHARED
			hairgeom.cpp
			hkd.cpp
			hbvh.cpp
			traverse.cpp
			)
		TARGET_LINK_LIBRARIES(ospray_module_hair ospray)

		ADD_EXECUTABLE(txt2hg
			txt2hg.cpp
			)
		TARGET_LINK_LIBRARIES(txt2hg ospray_module_hair)

		ADD_EXECUTABLE(hkdtest
			hkdtest.cpp
			)
		TARGET_LINK_LIBRARIES(hkdtest ospray_module_hair)

		ADD_EXECUTABLE(hg2hbvh
			hg2hbvh.cpp
			)
		TARGET_LINK_LIBRARIES(hg2hbvh ospray_module_hair)

		ADD_EXECUTABLE(hgstat
			hgstat.cpp
			)
		TARGET_LINK_LIBRARIES(hgstat ospray_module_hair)

		ADD_EXECUTABLE(hairViewer
			viewer.cpp
			)
		TARGET_LINK_LIBRARIES(hairViewer ospray_module_hair ospray_glut3d)
	ENDIF()
ENDIF()
