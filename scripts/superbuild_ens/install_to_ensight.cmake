set(ENSIGHT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)
IF (WIN32)
  set(PLATFORM win64)
  set(LIBSUFFIX ".lib")
ELSEIF(APPLE)
  set(PLATFORM apple)
  set(LIBSUFFIX ".dylib")  
ELSEIF(UNIX)
  set(PLATFORM linux_2.6_64)
  set(LIBSUFFIX ".so")  
ENDIF()
set(APEX_DIR ${ENSIGHT_SRC_DIR}/apex231/machines/${PLATFORM})
set(OSPRAY_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

####################embree install commands#######################
set(EMBREE_INSTALL_DIR ${APEX_DIR}/intel_embree-3.10.0)

IF (WIN32)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/embree/bin/embree3.dll
	DESTINATION ${EMBREE_INSTALL_DIR}/bin
)
ENDIF()

IF (WIN32)
  set(EMBREE_LIB_SRC_DIR ${CMAKE_BINARY_DIR}/embree/build/Release/)
  install(
	FILES ${EMBREE_LIB_SRC_DIR}embree3.lib
          ${EMBREE_LIB_SRC_DIR}embree_avx.lib
          ${EMBREE_LIB_SRC_DIR}embree_avx2.lib
          ${EMBREE_LIB_SRC_DIR}embree_sse42.lib
          ${EMBREE_LIB_SRC_DIR}lexers.lib
          ${EMBREE_LIB_SRC_DIR}math.lib
          ${EMBREE_LIB_SRC_DIR}simd.lib
          ${EMBREE_LIB_SRC_DIR}sys.lib
          ${EMBREE_LIB_SRC_DIR}tasking.lib			
	DESTINATION ${EMBREE_INSTALL_DIR}/lib/Release/
  )
  set(EMBREE_LIB_DEBSRC_DIR ${CMAKE_BINARY_DIR}/embree/build/Debug/)
  install(
	FILES ${EMBREE_LIB_DEBSRC_DIR}embree3.lib
          ${EMBREE_LIB_DEBSRC_DIR}embree_avx.lib
          ${EMBREE_LIB_DEBSRC_DIR}embree_avx2.lib
          ${EMBREE_LIB_DEBSRC_DIR}embree_sse42.lib
          ${EMBREE_LIB_DEBSRC_DIR}lexers.lib
          ${EMBREE_LIB_DEBSRC_DIR}math.lib
          ${EMBREE_LIB_DEBSRC_DIR}simd.lib
          ${EMBREE_LIB_DEBSRC_DIR}sys.lib
          ${EMBREE_LIB_DEBSRC_DIR}tasking.lib			
	DESTINATION ${EMBREE_INSTALL_DIR}/lib/Debug/
  )    
ELSEIF (APPLE)
  install(
	DIRECTORY ${CMAKE_INSTALL_PREFIX}/embree/lib
	DESTINATION ${EMBREE_INSTALL_DIR}/
  )  
ELSE()
  install(
	DIRECTORY ${CMAKE_INSTALL_PREFIX}/embree/lib64/
	DESTINATION ${EMBREE_INSTALL_DIR}/lib
  )    
ENDIF()

install(
	DIRECTORY ${CMAKE_BINARY_DIR}/embree/src/common
	          ${CMAKE_BINARY_DIR}/embree/src/include
	DESTINATION ${EMBREE_INSTALL_DIR}/
)


#####################tbb install commands########################
set(TBB_INSTALL_DIR ${APEX_DIR}/intel_tbb-2020)

IF (WIN32)
set(TBB_SRC_BIN_DIR ${CMAKE_INSTALL_PREFIX}/tbb/redist/intel64/vc14/)
install(
	FILES ${TBB_SRC_BIN_DIR}tbb12.dll
	      ${TBB_SRC_BIN_DIR}tbb12_debug.dll
		  ${TBB_SRC_BIN_DIR}tbbmalloc.dll
		  ${TBB_SRC_BIN_DIR}tbbmalloc_debug.dll
	DESTINATION ${TBB_INSTALL_DIR}/bin/
)
ENDIF()

IF (WIN32)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbb${LIBSUFFIX}
          ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbb12${LIBSUFFIX}
          ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbbmalloc${LIBSUFFIX}
	DESTINATION ${TBB_INSTALL_DIR}/lib/
)
install(
    FILES ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbb12${LIBSUFFIX}
    DESTINATION ${TBB_INSTALL_DIR}/lib/
    RENAME tbb12_debug${LIBSUFFIX}
)
install(
    FILES ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbbmalloc${LIBSUFFIX}
    DESTINATION ${TBB_INSTALL_DIR}/lib/
    RENAME tbbmalloc_debug${LIBSUFFIX}
)
ELSEIF (APPLE)
install(
    DIRECTORY ${CMAKE_INSTALL_PREFIX}/tbb/lib
    DESTINATION ${TBB_INSTALL_DIR}/
    FILES_MATCHING 
    PATTERN "libtbb*${LIBSUFFIX}*"
    PATTERN "libtbbmalloc*${LIBSUFFIX}*"
    PATTERN "*_debug*" EXCLUDE
    PATTERN "*_proxy*" EXCLUDE
    PATTERN "cmake*" EXCLUDE
)		  
ELSEIF (UNIX)
install(
	DIRECTORY ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/gcc4.8/
	DESTINATION ${TBB_INSTALL_DIR}/lib	
	FILES_MATCHING 
    PATTERN "libtbb${LIBSUFFIX}*"
    PATTERN "libtbbmalloc${LIBSUFFIX}*"
)		  
ENDIF()

install(
	DIRECTORY ${CMAKE_BINARY_DIR}/tbb/src/include
	DESTINATION ${TBB_INSTALL_DIR}/
)


###########################OIDN install commands###########################
set(OIDN_INSTALL_DIR ${APEX_DIR}/intel_oidn-1.2.1)

IF (WIN32)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/oidn/bin/OpenImageDenoise.dll
	DESTINATION ${OIDN_INSTALL_DIR}/bin/
)
ENDIF()

IF (WIN32)
set (OIDN_LIB_FILES ${CMAKE_INSTALL_PREFIX}/oidn/lib/OpenImageDenoise${LIBSUFFIX})
install(
	FILES ${OIDN_LIB_FILES}
	DESTINATION ${OIDN_INSTALL_DIR}/lib/
)  
ELSE()
install(
    DIRECTORY ${CMAKE_INSTALL_PREFIX}/oidn/lib
    DESTINATION ${OIDN_INSTALL_DIR}/
    FILES_MATCHING 
    PATTERN "libOpenImageDenoise*${LIBSUFFIX}*"
    PATTERN "cmake" EXCLUDE
    PATTERN "libtbb*" EXCLUDE
)
ENDIF()

install(
	DIRECTORY ${CMAKE_BINARY_DIR}/oidn/src/include
	DESTINATION ${OIDN_INSTALL_DIR}/
)


#############################ospray install commands########################
set(OSPRAY_INSTALL_DIR ${APEX_DIR}/intel_ospray-2.1.1)

IF (WIN32)
install(
	FILES ${CMAKE_INSTALL_PREFIX}/openvkl/bin/openvkl.dll
	      ${CMAKE_INSTALL_PREFIX}/openvkl/bin/openvkl_module_cpu_device.dll
	      ${CMAKE_INSTALL_PREFIX}/openvkl/bin/openvkl_module_cpu_device_4.dll
	      ${CMAKE_INSTALL_PREFIX}/openvkl/bin/openvkl_module_cpu_device_8.dll
	      ${CMAKE_INSTALL_PREFIX}/openvkl/bin/openvkl_module_cpu_device_16.dll
	      ${CMAKE_BINARY_DIR}/rkcommon/build/Release/rkcommon.dll
	      ${CMAKE_BINARY_DIR}/ospray/build/Release/ospray.dll
	      ${CMAKE_BINARY_DIR}/ospray/build/Release/ospray_module_cpu.dll
	DESTINATION ${OSPRAY_INSTALL_DIR}/bin/
)
ENDIF()

IF (WIN32)
install(
	FILES ${CMAKE_BINARY_DIR}/ospray/build/Release/ospray${LIBSUFFIX}
	      ${CMAKE_BINARY_DIR}/ospray/build/Release/ospray_module_cpu${LIBSUFFIX}
	DESTINATION ${OSPRAY_INSTALL_DIR}/lib/
)
ELSE()
install(
    DIRECTORY ${CMAKE_INSTALL_PREFIX}/openvkl/lib64/
    DESTINATION ${OSPRAY_INSTALL_DIR}/lib
    FILES_MATCHING
    PATTERN "lib*${LIBSUFFIX}*"
    PATTERN "CMakeFiles" EXCLUDE
    PATTERN "cmake" EXCLUDE
)
install(
    DIRECTORY ${CMAKE_INSTALL_PREFIX}/rkcommon/lib64/
    DESTINATION ${OSPRAY_INSTALL_DIR}/lib
    FILES_MATCHING
    PATTERN "lib*${LIBSUFFIX}*"
    PATTERN "CMakeFiles" EXCLUDE
    PATTERN "cmake" EXCLUDE
)
install(
#fixed symbol link copy issue
    DIRECTORY ${CMAKE_BINARY_DIR}/ospray/build/
    DESTINATION ${OSPRAY_INSTALL_DIR}/lib
    FILES_MATCHING
    PATTERN "lib*${LIBSUFFIX}*"
    PATTERN "CMakeFiles" EXCLUDE
    PATTERN "cmake" EXCLUDE
    PATTERN "ospray" EXCLUDE
    PATTERN "test*" EXCLUDE
    PATTERN "modules*" EXCLUDE
    PATTERN "libOpenImage*" EXCLUDE
    PATTERN "libembree*" EXCLUDE
    PATTERN "libtbb*" EXCLUDE
)

ENDIF()


install(
	DIRECTORY ${OSPRAY_SRC_DIR}/ospray/include
	DESTINATION ${OSPRAY_INSTALL_DIR}/
)


##########################openvkl install commands########################
#set(OPENVKL_INSTALL_DIR ${APEX_DIR}/intl_openvkl-1.1.0)
#install(
#	DIRECTORY ${CMAKE_INSTALL_PREFIX}/openvkl/
#	DESTINATION ${OPENVKL_INSTALL_DIR}/
#)

#########################rkcommon install commands########################
#set(RKCOMMON_INSTALL_DIR ${APEX_DIR}/intl_rkcommon-1.8.0)
#install(
#	DIRECTORY ${CMAKE_INSTALL_PREFIX}/rkcommon/
#	DESTINATION ${RKCOMMON_INSTALL_DIR}/
#)
