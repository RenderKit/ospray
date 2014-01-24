PROJECT(OSPRay)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

# whether to use ICC (vs GCC) for the non-MIC part
SET(OSPRAY_ICC ON CACHE BOOL "Use Intel Compiler?")
# whehter to build in MIC/xeon phi support
SET(OSPRAY_MIC ON CACHE BOOL "Build OSPRay with MIC Support?")
# whehter to build in MIC/xeon phi support
SET(OSPRAY_MPI OFF CACHE BOOL "Add MPI Remote/Distributed rendering support?")
# the arch we're targeting for the non-MIC/non-xeon phi part of ospray
SET(OSPRAY_XEON_TARGET "AVX2" CACHE STRING "Target ISA on host (SSE,AVX,AVX2)")


##############################################################
# modules
##############################################################

INCLUDE(cmake/embree.cmake)
INCLUDE(cmake/ospray.cmake)
INCLUDE(cmake/mpi.cmake)
INCLUDE(cmake/doxygen.cmake)
##############################################################
# the ospray library
##############################################################

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
SET (OSPRAY_TARGET ${OSPRAY_XEON_TARGET})
ADD_SUBDIRECTORY(ospray builddir/ospray/intel64)
ADD_SUBDIRECTORY(modules builddir/modules/intel64)
IF (OSPRAY_MIC)
	SET (OSPRAY_TARGET "MIC")
	ADD_SUBDIRECTORY(ospray builddir/ospray/mic)
	ADD_SUBDIRECTORY(ospray builddir/modules/mic)
ENDIF()

##############################################################
# ospray sample apps
##############################################################
SET (OSPRAY_TARGET ${OSPRAY_XEON_TARGET})
ADD_SUBDIRECTORY(apps builddir/apps/intel64)
#IF (OSPRAY_MIC)
#	SET (OSPRAY_TARGET "MIC")
#	ADD_SUBDIRECTORY(apps builddir/apps/mic)
#ENDIF()

