# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

IF (OSPRAY_MPI)
  if (OSPRAY_COMPILER STREQUAL "ICC")
    find_program(MPI_COMPILER 
      NAMES mpiicpc
      DOC "MPI compiler.")
    SET(OSPRAY_MPI_MULTI_THREADING_FLAG "-mt_mpi")
  else()
    find_package(MPI)
    if (${MPI_COMPILER} STREQUAL "MPI_COMPILER-NOTFOUND")
      #    message("MPI_COMPILER ${MPI_COMPILER}")
      #    message("MPI_Intel_COMPILER ${MPI_Intel_COMPILER}")
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
    if (${MPI_COMPILER} STREQUAL "MPI_COMPILER-NOTFOUND")
      message("could not find mpi compiler")
    endif()
    SET(OSPRAY_MPI_MULTI_THREADING_FLAG "") #???
  endif()
  mark_as_advanced(MPI_COMPILER)

  exec_program(${MPI_COMPILER} 
    ARGS -show
    OUTPUT_VARIABLE MPI_COMPILE_CMDLINE
    RETURN_VALUE MPI_COMPILER_RETURN)
  #message("MPI_COMPILE_CMDLINE ${MPI_COMPILE_CMDLINE}")

  # Extract include paths from compile command line
  string(REGEX MATCHALL "-I([^\" ]+|\"[^\"]+\")" MPI_ALL_INCLUDE_PATHS "${MPI_COMPILE_CMDLINE}")
  set(MPI_INCLUDE_PATH_WORK)
  foreach(IPATH ${MPI_ALL_INCLUDE_PATHS})
    string(REGEX REPLACE "^-I" "" IPATH ${IPATH})
    string(REGEX REPLACE "//" "/" IPATH ${IPATH})
    list(APPEND MPI_INCLUDE_PATH_WORK ${IPATH})
  endforeach(IPATH)

  set(MPI_INCLUDE_PATH ${MPI_INCLUDE_PATH_WORK} )
  # set(MPI_LIBRARIES_XEON
  # 	$ENV{I_MPI_ROOT}/intel64/lib/libmpi_mt.so
  # 	)
  # set(MPI_LIBRARIES_MIC
  # 	$ENV{I_MPI_ROOT}/mic/lib/libmpi_mt.so
  # 	)

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
      #			SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -mmic -mt_mpi" )



      #			SET(CMAKE_CXX_LINK_EXECUTABLE  	    "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} -mt_mpi -mmic -static-intel -rdynamic -fPIC -L${CMAKE_BINARY_DIR} <LINK_FLAGS> -o <TARGET> ${CMAKE_GNULD_IMAGE_VERSION} <OBJECTS> <LINK_LIBRARIES>")
      #			SET(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} -mt_mpi  -mmic -static-intel  <LANGUAGE_COMPILE_FLAGS> <CMAKE_SHARED_MODULE_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS> -o <TARGET> ${CMAKE_GNULD_IMAGE_VERSION} <OBJECTS> <LINK_LIBRARIES> -zmuldefs")
    ENDIF()

  ENDMACRO()

  IF (THIS_IS_MIC)
    # we _already_ included this in ospray, but at that time the "-mt_mpi" flag wasn't set"
    INCLUDE(${PROJECT_SOURCE_DIR}/cmake/icc_xeonphi.cmake)
    #		SET(CMAKE_CXX_LINK_EXECUTABLE  	    "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} -mmic -static-intel -rdynamic -fPIC -L${CMAKE_BINARY_DIR} <LINK_FLAGS> -o <TARGET> ${CMAKE_GNULD_IMAGE_VERSION} <OBJECTS> <LINK_LIBRARIES>")
    #		SET(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_CXX_COMPILER> ${CMAKE_CXX_FLAGS} -mmic -static-intel <LANGUAGE_COMPILE_FLAGS> <CMAKE_SHARED_MODULE_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS> -o <TARGET> ${CMAKE_GNULD_IMAGE_VERSION} <OBJECTS> <LINK_LIBRARIES> -zmuldefs")
  ENDIF()

  #	IF (THIS_IS_MIC)
  #		set(MPI_LIBRARIES_XEON ${MPI_LIBRARIES_MIC})
  #	ELSE()
  #		set(MPI_LIBRARIES_XEON ${MPI_LIBRARIES_XEON})
  #	ENDIF()
ELSE()
  MACRO(CONFIGURE_MPI)
    # nothing to do w/o mpi mode
  ENDMACRO()
ENDIF()