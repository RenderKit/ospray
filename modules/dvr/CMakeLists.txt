SET(MODULE_DVR OFF CACHE BOOL "Build 'DVR' (Direct Volume Rendering ray caster) module")

IF (MODULE_DVR)

	IF (NOT OSPRAY_MPI)
		MESSAGE("The 'DVR' module needs MPI support to be built in")
	ELSE()
		CONFIGURE_OSPRAY()
		CONFIGURE_MPI()

		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
		INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)

		OSPRAY_ADD_LIBRARY(ospray_module_dvr${OSPRAY_LIB_SUFFIX} SHARED
			dvr.cpp
			dvr.ispc
			)
		TARGET_LINK_LIBRARIES(ospray_module_dvr${OSPRAY_LIB_SUFFIX} ospray${OSPRAY_LIB_SUFFIX})

		IF (NOT THIS_IS_MIC)
			ADD_EXECUTABLE(dvrView dvrView)
			TARGET_LINK_LIBRARIES(dvrView ospray ospray_glut3d)
		ENDIF()
	ENDIF()
ENDIF()
