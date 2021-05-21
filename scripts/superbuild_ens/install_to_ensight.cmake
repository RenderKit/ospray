set(ENSIGHT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)
set(APEX_DIR ${ENSIGHT_SRC_DIR}/apex221/machines/win64)
set(OSPRAY_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

#embree install commands
set(EMBREE_INSTALL_DIR ${APEX_DIR}/intel_embree)

IF (WIN32)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/embree/bin/embree3.dll
	DESTINATION ${EMBREE_INSTALL_DIR}/bin
)
ENDIF()

install(
	DIRECTORY ${CMAKE_INSTALL_PREFIX}/embree/lib
	DESTINATION ${EMBREE_INSTALL_DIR}/
)

install(
	DIRECTORY ${CMAKE_BINARY_DIR}/embree/src/common
	DESTINATION ${EMBREE_INSTALL_DIR}/
)

install(
	DIRECTORY ${CMAKE_BINARY_DIR}/embree/src/include
	DESTINATION ${EMBREE_INSTALL_DIR}/
)


#tbb install commands
set(TBB_INSTALL_DIR ${APEX_DIR}/intel_tbb)

IF (WIN32)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/tbb/redist/intel64/vc14/tbb12.dll
		  ${CMAKE_INSTALL_PREFIX}/tbb/redist/intel64/vc14/tbbmalloc.dll
	DESTINATION ${TBB_INSTALL_DIR}/bin/
)
ENDIF()

IF (WIN32)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbb.lib
          ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbb12.lib
          ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbbmalloc.lib	
	DESTINATION ${TBB_INSTALL_DIR}/lib/
)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbb12.lib
	DESTINATION ${TBB_INSTALL_DIR}/lib/
	RENAME tbb12_debug.lib
)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbbmalloc.lib
	DESTINATION ${TBB_INSTALL_DIR}/lib/
	RENAME tbbmalloc_debug.lib
)
ELSEIF (APPLE)

ELSEIF (UNIX)

ENDIF()

install(
	DIRECTORY ${CMAKE_BINARY_DIR}/tbb/src/include
	DESTINATION ${TBB_INSTALL_DIR}/
)


#OIDN install commands
set(OIDN_INSTALL_DIR ${APEX_DIR}/intel_oidn)

IF (WIN32)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/oidn/bin/OpenImageDenoise.dll
	DESTINATION ${OIDN_INSTALL_DIR}/bin/
)
ENDIF()

IF (WIN32)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/oidn/lib/OpenImageDenoise.lib
	DESTINATION ${OIDN_INSTALL_DIR}/lib/
)
ELSEIF (APPLE)

ELSEIF (UNIX)

ENDIF()

install(
	DIRECTORY ${CMAKE_BINARY_DIR}/oidn/src/include
	DESTINATION ${OIDN_INSTALL_DIR}/
)


#ospray install commands
set(OSPRAY_INSTALL_DIR ${APEX_DIR}/intel_ospray)

IF (WIN32)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/openvkl/bin/openvkl.dll
	      ${CMAKE_INSTALL_PREFIX}/openvkl/bin/openvkl_module_ispc_driver.dll
	      ${CMAKE_INSTALL_PREFIX}/openvkl/bin/openvkl_module_ispc_driver_4.dll
	      ${CMAKE_INSTALL_PREFIX}/openvkl/bin/openvkl_module_ispc_driver_8.dll
	      ${CMAKE_INSTALL_PREFIX}/openvkl/bin/openvkl_module_ispc_driver_16.dll
	      ${CMAKE_INSTALL_PREFIX}/rkcommon/bin/rkcommon.dll
	      ${CMAKE_BINARY_DIR}/ospray/build/ospray.dll
	      ${CMAKE_BINARY_DIR}/ospray/build/ospray_module_ispc.dll
	DESTINATION ${OSPRAY_INSTALL_DIR}/bin/
)
ENDIF()

IF (WIN32)
install(
	FILES ${CMAKE_BINARY_DIR}/ospray/build/ospray.lib 
	      ${CMAKE_BINARY_DIR}/ospray/build/ospray_module_ispc.lib
	DESTINATION ${OSPRAY_INSTALL_DIR}/lib/
)
ELSEIF (APPLE)

ELSEIF (UNIX)

ENDIF()

install(
	DIRECTORY ${OSPRAY_SRC_DIR}/ospray/include
	DESTINATION ${OSPRAY_INSTALL_DIR}/
)


#openvkl install commands
set(OPENVKL_INSTALL_DIR ${APEX_DIR}/intl_openvkl)
install(
	DIRECTORY ${CMAKE_INSTALL_PREFIX}/openvkl/
	DESTINATION ${OPENVKL_INSTALL_DIR}/
)

#rkcommon install commands
set(RKCOMMON_INSTALL_DIR ${APEX_DIR}/intl_rkcommon)
install(
	DIRECTORY ${CMAKE_INSTALL_PREFIX}/rkcommon/
	DESTINATION ${RKCOMMON_INSTALL_DIR}/
)
