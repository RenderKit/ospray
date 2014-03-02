SET(MODULE_RV OFF CACHE BOOL "Build 'RV' (Resistor Visualation) module")

IF (MODULE_RV)
	CONFIGURE_OSPRAY()
	if (NOT THIS_IS_MIC)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include/)
		INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
		INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
		INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

		OSPRAY_ADD_LIBRARY(ospray_module_rv SHARED
			api.cpp
			geometry.cpp
			renderer.cpp
			renderer.ispc
			)
		TARGET_LINK_LIBRARIES(ospray_module_rv ospray)

		ADD_EXECUTABLE(ospRV
			viewer.cpp
			)
		TARGET_LINK_LIBRARIES(ospRV ospray_module_rv ospray_glut3d glui)
	ENDIF()
ENDIF()
