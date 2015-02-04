## ======================================================================== ##
## Copyright 2009-2014 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

IF (OSPRAY_MPI)
  FIND_PACKAGE(MPI)

  IF (NOT MPI_CXX_FOUND)
    MESSAGE(FATAL_ERROR "MPI C++ not found")
  ENDIF()

  IF (${MPI_CXX_COMPILER} STREQUAL "MPI_CXX_COMPILER-NOTFOUND")
    message(FATAL_ERROR "MPI C++ compiler not found")
  ENDIF()

  # need to find a way to set this as needed for different MPI versions.
  # -mt_mpi needed for Intel MPI.
  SET(OSPRAY_MPI_MULTI_THREADING_FLAG "")

  MACRO(CONFIGURE_MPI)
    INCLUDE_DIRECTORIES(${MPI_CXX_INCLUDE_PATH})
    SET(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MPI_CXX_COMPILE_FLAGS} ${OSPRAY_MPI_MULTI_THREADING_FLAG}")
    SET(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} ${MPI_CXX_COMPILE_FLAGS} ${OSPRAY_MPI_MULTI_THREADING_FLAG}")

    IF (THIS_IS_MIC)
      SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -mmic" )
    ENDIF()

  ENDMACRO()

  IF (THIS_IS_MIC)
    INCLUDE(${PROJECT_SOURCE_DIR}/cmake/icc_xeonphi.cmake)
  ENDIF()

ELSE()
  MACRO(CONFIGURE_MPI)
    # nothing to do w/o mpi mode
  ENDMACRO()
ENDIF()
