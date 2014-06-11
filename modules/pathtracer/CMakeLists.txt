SET(MODULE_PATHTRACER OFF CACHE BOOL "Build 'pathtracer' (Embree Path Tracer) module")

IF (MODULE_PATHTRACER)
	
	CONFIGURE_OSPRAY()
	CONFIGURE_MPI()
	
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
	INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
	INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
	INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
	INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)
	
	OSPRAY_ADD_LIBRARY(ospray_module_pathtracer${OSPRAY_LIB_SUFFIX} SHARED
		pathtracer.ispc
		pathtracer.cpp

		lights/light.ispc
		lights/ambientlight.ispc
		lights/pointlight.ispc
		lights/spotlight.ispc
		
		materials/material.ispc
		materials/obj.ispc
		materials/obj.cpp
		materials/velvet.ispc
		materials/velvet.cpp
		materials/metal.ispc
		materials/metal.cpp
		)
	TARGET_LINK_LIBRARIES(
		ospray_module_pathtracer${OSPRAY_LIB_SUFFIX} 
		ospray${OSPRAY_LIB_SUFFIX}
		)
ENDIF()
