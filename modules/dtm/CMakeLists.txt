SET(MODULE_DTM OFF CACHE BOOL "Build 'DTM' (Distibuted Triangle Meshes) module")

IF (MODULE_DTM)

	IF (NOT OSPRAY_MPI)
		MESSAGE("The 'DTM' module needs MPI support to be built in")
	ELSE()
		CONFIGURE_OSPRAY()
		CONFIGURE_MPI()

		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

		ADD_LIBRARY(ospray_dtm${OSPRAY_LIB_SUFFIX} SHARED
			dtm.cpp
			)

		# apps will not work on mic; only module will
		IF (NOT THIS_IS_MIC)
			ADD_EXECUTABLE(dtmViewer
				dtmViewer.cpp
				)
			TARGET_LINK_LIBRARIES(dtmViewer osp_glut3d ospray)
		ENDIF()
	ENDIF()
ENDIF()
