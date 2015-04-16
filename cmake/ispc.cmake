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

SET(ISPC_VERSION 1.8.1)

IF (APPLE)
  SET(OSPRAY_ISPC_DIRECTORY ${PROJECT_SOURCE_DIR}/../ispc-v${ISPC_VERSION}-osx CACHE STRING "ISPC Directory")
ELSE()
  SET(OSPRAY_ISPC_DIRECTORY ${PROJECT_SOURCE_DIR}/../ispc-v${ISPC_VERSION}-linux CACHE STRING "ISPC Directory")
ENDIF()
MARK_AS_ADVANCED(OSPRAY_ISPC_DIRECTORY)

IF (NOT EXISTS "${OSPRAY_ISPC_DIRECTORY}/ispc")
  MESSAGE("********************************************")
  MESSAGE("Could not find ISPC (tried ${OSPRAY_ISPC_DIRECTORY}.")
  MESSAGE("")
  MESSAGE("This version of ospray expects you to have a binary install of ISPC version ${ISPC_VERSION}, and expects this to be installed in the sibling directory to where the ospray source are located. Please go to https://ispc.github.io/downloads.html, select the binary release for your particular platform, and unpack it to ${PROJECT_SOURCE_DIR}/../")
  MESSAGE("")
  MESSAGE("If you insist on using your own custom install of ISPC, please make sure that the 'OSPRAY_ISPC_DIRECTORY' variable is properly set in cmake.")
  MESSAGE("********************************************")
  MESSAGE(FATAL_ERROR "Could not find ISPC. Exiting")
ELSE()
  SET(ISPC_BINARY ${OSPRAY_ISPC_DIRECTORY}/ispc)
ENDIF()

# ##################################################################
# add macro ADD_DEFINITIIONS_ISCP() that allows to pass #define's to
# ispc sources
# ##################################################################
SET (ISPC_DEFINES "")
MACRO (ADD_DEFINITIONS_ISPC)
  SET (ISPC_DEFINES ${ISPC_DEFINES} ${ARGN})
ENDMACRO ()

# ##################################################################
# add macro INCLUDE_DIRECTORIES_ISPC() that allows to specify search
# paths for ispc sources
# ##################################################################
SET (ISPC_INCLUDE_DIR "")
MACRO (INCLUDE_DIRECTORIES_ISPC)
  SET (ISPC_INCLUDE_DIR ${ISPC_INCLUDE_DIR} ${ARGN})
ENDMACRO ()

SET(ISPC_ADDRESSING 32 CACHE INT
  "32vs64 bit addressing in ispc")
MARK_AS_ADVANCED(ISPC_ADDRESSING)

MACRO(CONFIGURE_ISPC)
ENDMACRO()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
MACRO (ispc_compile)
  IF (CMAKE_OSX_ARCHITECTURES STREQUAL "i386")
    SET(ISPC_ARCHITECTURE "x86")
  ELSE()
    SET(ISPC_ARCHITECTURE "x86-64")
  ENDIF()
  
  IF(ISPC_INCLUDE_DIR)
    STRING(REGEX REPLACE ";" ";-I;" ISPC_INCLUDE_DIR_PARMS "${ISPC_INCLUDE_DIR}")
    SET(ISPC_INCLUDE_DIR_PARMS "-I" ${ISPC_INCLUDE_DIR_PARMS}) 
  ENDIF()

  IF (THIS_IS_MIC)
    SET(CMAKE_ISPC_FLAGS --opt=force-aligned-memory --target generic-16 --emit-c++ --c++-include-file=${PROJECT_SOURCE_DIR}/ospray/common/ISPC_KNC_Backend.h  --addressing=${ISPC_ADDRESSING})
    #${ISPC_DIR}/examples/intrinsics/knc.h)
    SET(ISPC_TARGET_EXT ".dev.cpp")
  ELSE()
    SET(CMAKE_ISPC_FLAGS --target=${OSPRAY_ISPC_TARGET} --addressing=${ISPC_ADDRESSING} --cpu=${OSPRAY_ISPC_CPU})
		SET(ISPC_TARGET_EXT ".dev.o")
  ENDIF()

	IF (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    SET(CMAKE_ISPC_OPT_FLAGS -O3)
	ELSE()
    SET(CMAKE_ISPC_OPT_FLAGS -O2 -g)
	ENDIF()

  SET(ISPC_OBJECTS "")
  FOREACH(src ${ARGN})
    SET(ISPC_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR})

    GET_FILENAME_COMPONENT(fname ${src} NAME_WE)
    GET_FILENAME_COMPONENT(dir ${src} PATH)

    IF ("${dir}" MATCHES "^/")
      # ------------------------------------------------------------------
      # global path name to input, like when we include the embree sources
      # from a global external embree checkout
      # ------------------------------------------------------------------
      STRING(REGEX REPLACE "^/" "${CMAKE_CURRENT_BINARY_DIR}/rebased/" outdir "${dir}")
      SET(indir "${dir}")
      SET(input "${indir}/${fname}.ispc")
    ELSE()
      # ------------------------------------------------------------------
      # local path name to input, like local ospray sources
      # ------------------------------------------------------------------
      SET(outdir "${CMAKE_CURRENT_BINARY_DIR}/local_${dir}")
      SET(indir "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
      SET(input "${indir}/${fname}.ispc")
    ENDIF()
    SET(outdirh ${ISPC_TARGET_DIR})

    SET(deps "")
    IF (EXISTS ${outdir}/${fname}.dev.idep)
      FILE(READ ${outdir}/${fname}.dev.idep contents)
      STRING(REGEX REPLACE " " ";"     contents "${contents}")
      STRING(REGEX REPLACE ";" "\\\\;" contents "${contents}")
      STRING(REGEX REPLACE "\n" ";"    contents "${contents}")
      FOREACH(dep ${contents})
	IF (EXISTS ${dep})
	  SET(deps ${deps} ${dep})
	ENDIF (EXISTS ${dep})
      ENDFOREACH(dep ${contents})
    ENDIF ()

    SET(ispc_compile_result "${outdir}/${fname}${ISPC_TARGET_EXT}")
    ADD_CUSTOM_COMMAND(
      OUTPUT ${ispc_compile_result} ${outdirh}/${fname}_ispc.h
      COMMAND mkdir -p ${outdir} \; ${ISPC_BINARY}
      -I ${CMAKE_CURRENT_SOURCE_DIR} 
      ${ISPC_INCLUDE_DIR_PARMS}
      ${ISPC_DEFINES}
      --arch=${ISPC_ARCHITECTURE}
      --pic
      --woff
      ${CMAKE_ISPC_FLAGS}
      ${CMAKE_ISPC_OPT_FLAGS}
      --opt=fast-math
      -h ${outdirh}/${fname}_ispc.h
      -MMM  ${outdir}/${fname}.dev.idep 
      -o ${ispc_compile_result}
      ${input}
      \;
      DEPENDS ${input}
      ${deps})
    SET(ISPC_OBJECTS ${ISPC_OBJECTS} ${ispc_compile_result})

  ENDFOREACH()

ENDMACRO()


MACRO (add_ispc_executable name)
  SET(ISPC_SOURCES "")
  SET(OTHER_SOURCES "")
  FOREACH(src ${ARGN})
    GET_FILENAME_COMPONENT(ext ${src} EXT)
    IF (ext STREQUAL ".ispc")
      SET(ISPC_SOURCES ${ISPC_SOURCES} ${src})
    ELSE ()
      SET(OTHER_SOURCES ${OTHER_SOURCES} ${src})
    ENDIF ()
  ENDFOREACH()
  ISPC_COMPILE(${ISPC_SOURCES})
  ADD_EXECUTABLE(${name} ${ISPC_OBJECTS} ${OTHER_SOURCES})
ENDMACRO()

MACRO (OSPRAY_ADD_LIBRARY name type)
  SET(ISPC_SOURCES "")
  SET(OTHER_SOURCES "")
  SET(ISPC_OBJECTS "")
  FOREACH(src ${ARGN})
    GET_FILENAME_COMPONENT(ext ${src} EXT)
    IF (ext STREQUAL ".ispc")
      SET(ISPC_SOURCES ${ISPC_SOURCES} ${src})
    ELSE ()
      SET(OTHER_SOURCES ${OTHER_SOURCES} ${src})
    ENDIF ()
  ENDFOREACH()
  ISPC_COMPILE(${ISPC_SOURCES})
  ADD_LIBRARY(${name} ${type} ${ISPC_OBJECTS} ${OTHER_SOURCES})
ENDMACRO()
