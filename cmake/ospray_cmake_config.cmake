## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
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

# Version header
configure_file(
  ${CMAKE_SOURCE_DIR}/ospray/version.h.in
  ${CMAKE_BINARY_DIR}/ospray/version.h
  @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/ospray/version.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray
  COMPONENT devel
)

# CMake config module
set(OSPRAY_MODULE_CONFIG_INPUT_DIR  ${CMAKE_SOURCE_DIR}/cmake/ospray_cmake_config)
set(OSPRAY_MODULE_CONFIG_OUTPUT_DIR ${CMAKE_BINARY_DIR}/cmake)

set(OSPRAY_MODULE_FILES
  osprayConfig.cmake
  osprayConfigVersion.cmake
  osprayUse.cmake
)

## find relative path to make package relocatable
# this is a bit involved to handle these cases:
#   - CMAKE_INSTALL_LIBDIR is overridden by the user
#   - CMAKE_INSTALL_LIBDIR contains multiple levels for Debian multiarch support
if (IS_ABSOLUTE "${CMAKE_INSTALL_PREFIX}")
  set(ABS_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
else()
  get_filename_component(ABS_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
endif()

if (IS_ABSOLUTE "${OSPRAY_CMAKECONFIG_DIR}")
  MESSAGE(WARNING "CMAKE_INSTALL_LIBDIR needs to be a relative path!")
  set(ABS_CMAKECONFIG_DIR "${OSPRAY_CMAKECONFIG_DIR}")
else()
  set(ABS_CMAKECONFIG_DIR "${ABS_INSTALL_PREFIX}/${OSPRAY_CMAKECONFIG_DIR}")
endif()

file(RELATIVE_PATH OSPRAY_RELATIV_ROOT_DIR "${ABS_CMAKECONFIG_DIR}" "${ABS_INSTALL_PREFIX}" )


foreach(MODULE_FILE ${OSPRAY_MODULE_FILES})
  LIST(APPEND OSPRAY_MODULE_INSTALL_FILES
    ${OSPRAY_MODULE_CONFIG_OUTPUT_DIR}/${MODULE_FILE}
  )

  configure_file(
    ${OSPRAY_MODULE_CONFIG_INPUT_DIR}/${MODULE_FILE}.in
    ${OSPRAY_MODULE_CONFIG_OUTPUT_DIR}/${MODULE_FILE}
    @ONLY
  )
endforeach()

install(FILES
  ${OSPRAY_MODULE_INSTALL_FILES}
  ${CMAKE_SOURCE_DIR}/components/ospcommon/cmake/FindTBB.cmake
  ${CMAKE_SOURCE_DIR}/components/ospcommon/cmake/clang.cmake
  ${CMAKE_SOURCE_DIR}/components/ospcommon/cmake/icc.cmake
  ${CMAKE_SOURCE_DIR}/components/ospcommon/cmake/ispc.cmake
  ${CMAKE_SOURCE_DIR}/components/ospcommon/cmake/gcc.cmake
  ${CMAKE_SOURCE_DIR}/components/ospcommon/cmake/msvc.cmake
  ${CMAKE_SOURCE_DIR}/components/ospcommon/cmake/macros.cmake#NOTE(jda) - hack!
  ${CMAKE_SOURCE_DIR}/cmake/ospray_macros.cmake
  DESTINATION ${OSPRAY_CMAKECONFIG_DIR}
  COMPONENT devel
)
