SET(MODULE_HAIR OFF CACHE BOOL "Build 'Hair' module")

IF (MODULE_HAIR)
	CONFIGURE_OSPRAY()
	if (NOT THIS_IS_MIC)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)

		ADD_LIBRARY(hair
			hairgeom.cpp
			hkd.cpp
			)
		TARGET_LINK_LIBRARIES(hair ospray)

#		ADD_EXECUTABLE(hair_tessellate
#			tessellate.cpp
#			)
#		TARGET_LINK_LIBRARIES(hair_tessellate ospray)

		ADD_EXECUTABLE(txt2hg
			txt2hg.cpp
			)
		TARGET_LINK_LIBRARIES(txt2hg hair)

		ADD_EXECUTABLE(hkdtest
			hkdtest.cpp
			)
		TARGET_LINK_LIBRARIES(hkdtest hair)

		ADD_EXECUTABLE(hgstat
			hgstat.cpp
			)
		TARGET_LINK_LIBRARIES(hgstat hair)
	ENDIF()
ENDIF()
