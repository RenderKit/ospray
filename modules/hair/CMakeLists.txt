SET(MODULE_HAIR OFF CACHE BOOL "Build 'Hair' module")

IF (MODULE_HAIR)
	CONFIGURE_OSPRAY()
	if (NOT THIS_IS_MIC)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
		ADD_EXECUTABLE(hair_tessellate
			tessellate.cpp
			)
		TARGET_LINK_LIBRARIES(hair_tessellate ospray)
	ENDIF()
ENDIF()
