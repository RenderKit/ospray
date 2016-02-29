## ======================================================================== ##
## Copyright 2009-2016 Intel Corporation                                    ##
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

OPTION(OSPRAY_MODULE_TACHYON "Build 'TACH' (tachyon model viewer)")

IF (OSPRAY_MODULE_TACHYON)

  CONFIGURE_OSPRAY()

  # -------------------------------------------------------
  find_program(LEX_EXE flex)
  if(LEX_EXE STREQUAL "LEX_EXE-NOTFOUND")
    message(FATAL_ERROR "dear user, plase install flex!")
  endif(LEX_EXE STREQUAL "LEX_EXE-NOTFOUND")

  find_program(YACC_EXE bison)
  if(YACC_EXE STREQUAL "YACC_EXE-NOTFOUND")
    message(FATAL_ERROR "dear user, plase install bison!")
  endif(YACC_EXE STREQUAL "YACC_EXE-NOTFOUND")

  # reuseable cmake macro for yacc
  MACRO(YACC_FILE _filename)
    GET_FILENAME_COMPONENT(_basename ${_filename} NAME_WE)
    ADD_CUSTOM_COMMAND(
      OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_y.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_y.hpp
      COMMAND ${YACC_EXE} -d --debug --verbose
      --output=${_basename}_y.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/${_filename}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_filename}
      )
  ENDMACRO(YACC_FILE)

  # reuseable cmake macro for lex
  MACRO(LEX_FILE _filename)
    GET_FILENAME_COMPONENT(_basename ${_filename} NAME_WE)
    ADD_CUSTOM_COMMAND(
      OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_l.cpp
      COMMAND ${LEX_EXE}
      -o${_basename}_l.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/${_filename}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_filename} )
  ENDMACRO(LEX_FILE)
  # -------------------------------------------------------

  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/)
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

  YACC_FILE(parser.yy)
  LEX_FILE(lexer.ll)

  OSPRAY_ADD_LIBRARY(ospray_module_tachyon SHARED
    TachyonRenderer.cpp
    TachyonRenderer.ispc
  )

  OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_module_tachyon ospray)

  IF (NOT THIS_IS_MIC)
    ADD_EXECUTABLE(ospTachyon 
      Viewer 
      Model
      lexer_l.cpp
      parser_y.cpp
      Loc.cpp
    )
    TARGET_LINK_LIBRARIES(ospTachyon ospray ospray_glut3d)
  ENDIF()
ENDIF()
