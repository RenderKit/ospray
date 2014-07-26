# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(OSPRAY_APPS_PARTICLEVIEWER OFF CACHE BOOL "Build ospParticleViewer viewer")

IF (OSPRAY_APPS_PARTICLEVIEWER)

  CONFIGURE_OSPRAY()

  # -------------------------------------------------------
  find_program(LEX_EXE
    flex
    )
  if(LEX_EXE STREQUAL "LEX_EXE-NOTFOUND")
    message(FATAL_ERROR "dear user, plase install flex!")
  endif(LEX_EXE STREQUAL "LEX_EXE-NOTFOUND")

  find_program(YACC_EXE
    bison
    )
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
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/apps/util)
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
  INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
  INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

  IF (NOT THIS_IS_MIC)
    YACC_FILE(parser.yy)
    LEX_FILE(lexer.ll)

    ADD_EXECUTABLE(ospLammpsView 
      viewer.cpp
      model.cpp
      loc.cpp
      lexer_l.cpp
      parser_y.cpp
      )
    TARGET_LINK_LIBRARIES(ospLammpsView ospray ospray_glut3d)
  ENDIF()
ENDIF()
