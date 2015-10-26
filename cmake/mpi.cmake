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
  #  OPTION(OSPRAY_FORCE_IMPI "Force use of Intel MPI" OFF)
  SET(OSPRAY_MPI_DISTRIBUTED ON) # sets this define in OSPConfig.h

  MACRO(CONFIGURE_MPI)
    IF (THIS_IS_MIC)
      SET(CMAKE_MPI_MIC_FLAG "-mmic")
    ELSE()
      SET(CMAKE_MPI_MIC_FLAG "")
    ENDIF()
    IF (WIN32)
      FIND_PROGRAM(MPI_COMPILER NAMES mpiicpc.bat DOC "MPI compiler wrapper.")
    ENDIF()
    IF (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
      #  IF (OSPRAY_FORCE_IMPI)
      SET(MPI_CXX_COMPILER "mpiicpc")
      execute_process(
	COMMAND ${MPI_CXX_COMPILER} -mt_mpi -show  ${CMAKE_MPI_MIC_FLAG}
	OUTPUT_VARIABLE  MPI_COMPILE_CMDLINE OUTPUT_STRIP_TRAILING_WHITESPACE
	ERROR_VARIABLE   MPI_COMPILE_CMDLINE ERROR_STRIP_TRAILING_WHITESPACE
	RESULT_VARIABLE  MPI_COMPILER_RETURN)
      set(MPI_LINK_CMDLINE ${MPI_COMPILE_CMDLINE})
      #   message("MPI_COMPILE_CMDLINE ${MPI_COMPILE_CMDLINE}")
      #  message("MPI_LINK_CMDLINE ${MPI_LINK_CMDLINE}")
      
      # Extract compile flags from the compile command line.
      string(REGEX MATCHALL "(^| )-[Df]([^\" ]+|\"[^\"]+\")" MPI_ALL_COMPILE_FLAGS "${MPI_COMPILE_CMDLINE}")
      set(MPI_COMPILE_FLAGS_WORK)
      
      foreach(FLAG ${MPI_ALL_COMPILE_FLAGS})
	if (MPI_COMPILE_FLAGS_WORK)
          set(MPI_COMPILE_FLAGS_WORK "${MPI_COMPILE_FLAGS_WORK} ${FLAG}")
	else()
          set(MPI_COMPILE_FLAGS_WORK ${FLAG})
	endif()
      endforeach()
      
      # Extract include paths from compile command line
      string(REGEX MATCHALL "(^| )-I([^\" ]+|\"[^\"]+\")" MPI_ALL_INCLUDE_PATHS "${MPI_COMPILE_CMDLINE}")
      foreach(IPATH ${MPI_ALL_INCLUDE_PATHS})
	string(REGEX REPLACE "^ ?-I" "" IPATH ${IPATH})
        string(REPLACE "//" "/" IPATH ${IPATH})
        string(REPLACE "\"" "" IPATH ${IPATH})
	list(APPEND MPI_INCLUDE_PATH_WORK ${IPATH})
      endforeach()
      
      # try using showme:incdirs if extracting didn't work.
      if (NOT MPI_INCLUDE_PATH_WORK)
	set(MPI_INCLUDE_PATH_WORK ${MPI_INCDIRS})
	separate_arguments(MPI_INCLUDE_PATH_WORK)
      endif()
      
      # If all else fails, just search for mpi.h in the normal include paths.
      if (NOT MPI_INCLUDE_PATH_WORK)
	set(MPI_HEADER_PATH "MPI_HEADER_PATH-NOTFOUND" CACHE FILEPATH "Cleared" FORCE)
	find_path(MPI_HEADER_PATH mpi.h
          HINTS ${_MPI_BASE_DIR} ${_MPI_PREFIX_PATH}
          PATH_SUFFIXES include)
	set(MPI_INCLUDE_PATH_WORK ${MPI_HEADER_PATH})
      endif()
      
      # Extract linker paths from the link command line
      string(REGEX MATCHALL "(^| |-Wl,)-L([^\" ]+|\"[^\"]+\")" MPI_ALL_LINK_PATHS "${MPI_LINK_CMDLINE}")
      set(MPI_LINK_PATH)
      foreach(LPATH ${MPI_ALL_LINK_PATHS})
	string(REGEX REPLACE "^(| |-Wl,)-L" "" LPATH ${LPATH})
	string(REGEX REPLACE "//" "/" LPATH ${LPATH})
	list(APPEND MPI_LINK_PATH ${LPATH})
      endforeach()
      #    message("MPI_ALL_LINK_PATHS ${MPI_ALL_LINK_PATHS}")
      
      # try using showme:libdirs if extracting didn't work.
      if (NOT MPI_LINK_PATH)
	set(MPI_LINK_PATH ${MPI_LIBDIRS})
	separate_arguments(MPI_LINK_PATH)
      endif()
      
      # Extract linker flags from the link command line
      string(REGEX MATCHALL "(^| )-Wl,([^\" ]+|\"[^\"]+\")" MPI_ALL_LINK_FLAGS "${MPI_LINK_CMDLINE}")
      set(MPI_LINK_FLAGS_WORK)
      foreach(FLAG ${MPI_ALL_LINK_FLAGS})
	if (MPI_LINK_FLAGS_WORK)
          set(MPI_LINK_FLAGS_WORK "${MPI_LINK_FLAGS_WORK} ${FLAG}")
	else()
          set(MPI_LINK_FLAGS_WORK ${FLAG})
	endif()
      endforeach()
      
      # Extract the set of libraries to link against from the link command
      # line
      string(REGEX MATCHALL "(^| )-l([^\" ]+|\"[^\"]+\")" MPI_LIBNAMES "${MPI_LINK_CMDLINE}")
      # add the compiler implicit directories because some compilers
      # such as the intel compiler have libraries that show up
      # in the showme list that can only be found in the implicit
      # link directories of the compiler. Do this for C++ and C
      # compilers if the implicit link directories are defined.
      if (DEFINED CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES)
	set(MPI_LINK_PATH
          "${MPI_LINK_PATH};${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES}")
      endif ()
      
      if (DEFINED CMAKE_C_IMPLICIT_LINK_DIRECTORIES)
	set(MPI_LINK_PATH
          "${MPI_LINK_PATH};${CMAKE_C_IMPLICIT_LINK_DIRECTORIES}")
      endif ()
      
      #    message("MPI_LIBNAMES ${MPI_LIBNAMES}")
      # Determine full path names for all of the libraries that one needs
      # to link against in an MPI program
      foreach(LIB ${MPI_LIBNAMES})
	string(REGEX REPLACE "^ ?-l" "" LIB ${LIB})
	# MPI_LIB is cached by find_library, but we don't want that.  Clear it first.
	set(MPI_LIB "MPI_LIB-NOTFOUND" CACHE FILEPATH "Cleared" FORCE)
	find_library(MPI_LIB NAMES ${LIB} HINTS ${MPI_LINK_PATH})
	
	if (MPI_LIB)
          list(APPEND MPI_LIBRARIES_WORK ${MPI_LIB})
	elseif (NOT MPI_FIND_QUIETLY)
          message(WARNING "Unable to find MPI library ${LIB}")
	endif()
      endforeach()
      #    message("MPI_LIBRARIES_WORK ${MPI_LIBRARIES_WORK}")

      # Sanity check MPI_LIBRARIES to make sure there are enough libraries
      list(LENGTH MPI_LIBRARIES_WORK MPI_NUMLIBS)
      list(LENGTH MPI_LIBNAMES MPI_NUMLIBS_EXPECTED)
      if (NOT MPI_NUMLIBS EQUAL MPI_NUMLIBS_EXPECTED)
	set(MPI_LIBRARIES_WORK "MPI_${lang}_LIBRARIES-NOTFOUND")
      endif()
      SET(MPI_CXX_FOUND true)
      SET(MPI_CXX_COMPILE_FLAGS "-mt_mpi")
      SET(MPI_CXX_LINK_FLAGS "-mt_mpi")
      SET(MPI_CXX_LIBRARIES ${MPI_LIBRARIES_WORK})
      SET(MPI_LIBRARIES ${MPI_LIBRARIES_WORK})
    ELSE()
      FIND_PACKAGE(MPI)
    ENDIF()
    
    IF (NOT MPI_CXX_FOUND)
      MESSAGE(FATAL_ERROR "MPI C++ not found")
    ENDIF()

    IF (${MPI_CXX_COMPILER} STREQUAL "MPI_CXX_COMPILER-NOTFOUND")
      message(FATAL_ERROR "MPI C++ compiler not found")
    ENDIF()

    INCLUDE_DIRECTORIES(${MPI_CXX_INCLUDE_PATH})
    IF (WIN32)
      STRING(REGEX MATCHALL "/LIBPATH:([^\" ]+|\"[^\"]+\")" MPI_ALL_LIB_PATHS "${MPI_COMPILE_CMDLINE}")
      STRING(REPLACE ";" " " MPI_ALL_LIB_PATHS "${MPI_ALL_LIB_PATHS}")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MPI_ALL_LIB_PATHS} impi.lib  impicxx.lib")
        SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MPI_ALL_LIB_PATHS} impi.lib  impicxx.lib")
    ENDIF()
    SET(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MPI_CXX_COMPILE_FLAGS}")
    SET(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} ${MPI_CXX_COMPILE_FLAGS}")

    IF (THIS_IS_MIC)
      SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_MPI_MIC_FLAG}" )
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
