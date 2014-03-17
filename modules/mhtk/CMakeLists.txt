SET(MODULE_MHTK OFF CACHE BOOL "Build 'MHTK' module")

IF (MODULE_MHTK)
	CONFIGURE_OSPRAY()
	if (NOT THIS_IS_MIC)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
		INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

		INCLUDE_DIRECTORIES(${EMBREE_DIR}/kernels/xeon/)
		INCLUDE_DIRECTORIES(${EMBREE_DIR}/tutorials/common)

		INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
		INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

		OSPRAY_ADD_LIBRARY(ospray_module_mhtk SHARED
			multihit_kernel.cpp
			multihit_kernel.ispc

			xray_renderer.cpp
			xray_renderer.ispc
			)
		TARGET_LINK_LIBRARIES(ospray_module_mhtk ospray)
	ENDIF()
ENDIF()
