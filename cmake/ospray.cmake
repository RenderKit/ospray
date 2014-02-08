FILE(WRITE "${CMAKE_BINARY_DIR}/CMakeDefines.h" "#define CMAKE_BUILD_DIR \"${CMAKE_BINARY_DIR}\"\n")

#include bindir - that's where ispc puts generated header files
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
SET(OSPRAY_BINARY_DIR ${CMAKE_BINARY_DIR})




# Configure the output directories. To allow IMPI to do its magic we
# will put *exexecutables* into the (same) build directory, but tag
# mic-executables with ".mic". *libraries* cannot use the
# ".mic"-suffix trick, so we'll put libraries into separate
# directories (names 'intel64' and 'mic', respectively)
MACRO(CONFIGURE_OSPRAY)
	SET(EXECUTABLE_OUTPUT_PATH "${OSPRAY_BINARY_DIR}")
	IF (OSPRAY_TARGET STREQUAL "MIC")
		SET(OSPRAY_EXE_SUFFIX ".mic")
		SET(OSPRAY_LIB_SUFFIX "_mic")
		SET(OSPRAY_ISPC_SUFFIX ".cpp")
		SET(OSPRAY_ISPC_TARGET "mic")
		SET(THIS_IS_MIC ON)
		SET(__XEON__ OFF)
		#		SET(LIBRARY_OUTPUT_PATH "${OSPRAY_BINARY_DIR}/lib/mic")
		ADD_DEFINITIONS(-DOSPRAY_SPMD_WIDTH=16)
		IF (ISPC_PLUS)
			INCLUDE_DIRECTORIES(/home/wald/Projects/ISPC++)
		ENDIF()
	ELSE()
		SET(OSPRAY_EXE_SUFFIX "")
		SET(OSPRAY_LIB_SUFFIX "")
		IF (ISPC_PLUS)
			SET(OSPRAY_ISPC_SUFFIX ".cpp")
			INCLUDE_DIRECTORIES(/home/wald/Projects/ISPC++)
		ELSE()
			SET(OSPRAY_ISPC_SUFFIX ".o")
		ENDIF()
		SET(THIS_IS_MIC OFF)
		SET(__XEON__ ON)
		IF (OSPRAY_ICC)
			INCLUDE(${EMBREE_DIR}/common/cmake/icc.cmake)
		ELSE()
			INCLUDE(${EMBREE_DIR}/common/cmake/gcc.cmake)
		ENDIF()

		IF (${OSPRAY_XEON_TARGET} STREQUAL "AVX2")
			ADD_DEFINITIONS(-DOSPRAY_SPMD_WIDTH=8)
			SET(OSPRAY_ISPC_TARGET "avx2")
		ELSEIF (${OSPRAY_XEON_TARGET} STREQUAL "AVX")
			ADD_DEFINITIONS(-DOSPRAY_SPMD_WIDTH=8)
			SET(OSPRAY_ISPC_TARGET "avx")
		ELSEIF (${OSPRAY_XEON_TARGET} STREQUAL "SSE")
			ADD_DEFINITIONS(-DOSPRAY_SPMD_WIDTH=4)
			SET(OSPRAY_ISPC_TARGET "sse4")
		ELSE()
			MESSAGE("unknown OSPRAY_XEON_TARGET '${OSPRAY_XEON_TARGET}'")
		ENDIF()

	ENDIF()
	
	IF (OSPRAY_MPI)
		ADD_DEFINITIONS(-DOSPRAY_MPI=1)
	ENDIF()

	IF (THIS_IS_MIC)
		INCLUDE(${EMBREE_DIR}/common/cmake/icc_xeonphi.cmake)
		SET(EMBREE_LIB embree_xeonphi)
	ELSE()
		SET(EMBREE_LIB embree)
	ENDIF()
ENDMACRO()


