## ======================================================================== ##
## Copyright 2009-2015 Intel Corporation                                    ##
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
  if (OSPRAY_COMPILER STREQUAL "ICC")
    find_program(MPI_COMPILER 
      NAMES mpiicpc
      DOC "MPI compiler.")
    SET(OSPRAY_MPI_MULTI_THREADING_FLAG "-mt_mpi")
  else()
    find_package(MPI)
    if (${MPI_COMPILER} STREQUAL "MPI_COMPILER-NOTFOUND")
      find_program(MPI_COMPILER
        NAMES mpicxx
        PATHS /usr/lib64/openmpi/bin
        DOC "MPI compiler.")
      find_library(MPI_LIBRARY
        NAMES mpi
        PATHS /usr/lib64/openmpi/lib
        DOC "MPI library.")
      SET(MPI_LIBRARIES ${MPI_LIBRARY})
    endif()
    SET(OSPRAY_MPI_MULTI_THREADING_FLAG "") #???
  endif()
  if (${MPI_COMPILER} STREQUAL "MPI_COMPILER-NOTFOUND")
    message("could not find mpi compiler")
  endif()
  mark_as_advanced(MPI_COMPILER)

  exec_program(${MPI_COMPILER} 
    ARGS -show
    OUTPUT_VARIABLE MPI_COMPILE_CMDLINE
    RETURN_VALUE MPI_COMPILER_RETURN)

  # Extract include paths from compile command line
  string(REGEX MATCHALL "-I([^\" ]+|\"[^\"]+\")" MPI_ALL_INCLUDE_PATHS "${MPI_COMPILE_CMDLINE}")
  set(MPI_INCLUDE_PATH_WORK)
  foreach(IPATH ${MPI_ALL_INCLUDE_PATHS})
    string(REGEX REPLACE "^-I" "" IPATH ${IPATH})
    string(REGEX REPLACE "//" "/" IPATH ${IPATH})
    list(APPEND MPI_INCLUDE_PATH_WORK ${IPATH})
  endforeach(IPATH)

  set(MPI_INCLUDE_PATH ${MPI_INCLUDE_PATH_WORK} )

  MACRO(CONFIGURE_MPI)
    INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
    SET(CMAKE_CXX_COMPILER ${MPI_COMPILER})
    SET(APP_CMAKE_CXX_FLAGS "${OSPRAY_MPI_MULTI_THREADING_FLAG}")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    SET(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OSPRAY_MPI_MULTI_THREADING_FLAG}")
    SET(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} ${OSPRAY_MPI_MULTI_THREADING_FLAG}")

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