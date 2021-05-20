set(ENSIGHT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)
set(APEX_DIR ${ENSIGHT_SRC_DIR}/apex221/machines/win64)

#embree install commands
set(EMBREE_INSTALL_DIR ${APEX_DIR}/intel_embree)

install(
	FILES ${CMAKE_INSTALL_PREFIX}/embree/bin/embree3.dll
	DESTINATION ${EMBREE_INSTALL_DIR}/bin
)

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

install(
	FILES ${CMAKE_INSTALL_PREFIX}/tbb/redist/intel64/vc14/tbb12.dll
		  ${CMAKE_INSTALL_PREFIX}/tbb/redist/intel64/vc14/tbbmalloc.dll
	DESTINATION ${TBB_INSTALL_DIR}/bin/
)

install(
	FILES ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbb.lib
          ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbb12.lib
          ${CMAKE_INSTALL_PREFIX}/tbb/lib/intel64/vc14/tbbmalloc.lib	
	DESTINATION ${TBB_INSTALL_DIR}/lib/
)

install(
	DIRECTORY ${CMAKE_BINARY_DIR}/tbb/src/include
	DESTINATION ${TBB_INSTALL_DIR}/
)
