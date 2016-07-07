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

include_directories(${CMAKE_CURRENT_LIST_DIR})

set(LIBRARY_NAME ospray_commandline)

option(OSPRAY_COMMANDLINE_TACHYON_SUPPORT OFF)
mark_as_advanced(OSPRAY_COMMANDLINE_TACHYON_SUPPORT)

if(OSPRAY_COMMANDLINE_TACHYON_SUPPORT)
  # -------------------------------------------------------
  find_program(LEX_EXE flex)
  if(LEX_EXE STREQUAL "LEX_EXE-NOTFOUND")
    message(FATAL_ERROR "dear user, plase install flex!")
  endif()

  find_program(YACC_EXE bison)
  if(YACC_EXE STREQUAL "YACC_EXE-NOTFOUND")
    message(FATAL_ERROR "dear user, plase install bison!")
  endif()

  # reuseable cmake macro for yacc
  macro(yacc_file _filename)
    get_filename_component(_basename ${_filename} NAME_WE)
    add_custom_command(
      OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_y.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_y.hpp
      COMMAND ${YACC_EXE} -d --debug --verbose
      --output=${_basename}_y.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/${_filename}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_filename}
      )
  endmacro()

  # reuseable cmake macro for lex
  macro(lex_file _filename)
    get_filename_component(_basename ${_filename} NAME_WE)
    add_custom_command(
      OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_l.cpp
      COMMAND ${LEX_EXE}
      -o${_basename}_l.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/${_filename}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_filename} )
  endmacro()
  # -------------------------------------------------------

  yacc_file(SceneParser/tachyon/parser.yy)
  lex_file(SceneParser/tachyon/lexer.ll)

  add_definitions(-DOSPRAY_TACHYON_SUPPORT)
endif()

set(SOURCE_FILES
  CommandLineExport.h
  CommandLineParser.h
  CameraParser.cpp
  LightsParser.cpp
  RendererParser.cpp

  SceneParser/SceneParser.h
  SceneParser/MultiSceneParser.cpp

  SceneParser/particle/ParticleSceneParser.cpp
  SceneParser/particle/Model.cpp
  SceneParser/particle/uintah.cpp

  SceneParser/streamlines/StreamLineSceneParser.cpp

  SceneParser/trianglemesh/TriangleMeshSceneParser.cpp

  Utility.h
)

if(OSPRAY_COMMANDLINE_TACHYON_SUPPORT)
  set(SOURCE_FILES
    ${SOURCE_FILES}
    SceneParser/tachyon/TachyonSceneParser.cpp
    SceneParser/tachyon/Model.cpp
    SceneParser/tachyon/Loc.cpp
    lexer_l.cpp
    parser_y.cpp
  )
endif()

if(NOT WIN32)
  set(SOURCE_FILES
    ${SOURCE_FILES}
    SceneParser/volume/VolumeSceneParser.cpp
  )
endif()

add_library(${LIBRARY_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${LIBRARY_NAME}
  ospray_common
  ospray_minisg
  ospray_xml
)

if(NOT WIN32)
  target_link_libraries(${LIBRARY_NAME} ospray_importer)
endif()

ospray_set_library_version(${LIBRARY_NAME})

install(TARGETS ${LIBRARY_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT devel
)
