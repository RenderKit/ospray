PROJECT(OSPRay)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING
		"Choose the build type: Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)


##############################################################
# modules
##############################################################

INCLUDE(cmake/ospray.cmake)
INCLUDE(cmake/mpi.cmake)
INCLUDE(cmake/doxygen.cmake)
##############################################################
# the ospray library
##############################################################

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
SET (OSPRAY_TARGET ${OSPRAY_XEON_TARGET})
ADD_SUBDIRECTORY(ospray builddir/ospray/intel64)
IF (OSPRAY_MIC)
	SET (OSPRAY_TARGET "MIC")
	ADD_SUBDIRECTORY(ospray builddir/ospray/mic)
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

