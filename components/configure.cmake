## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
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

# lits of components that have already been built. we use this for
# dependencies tracking: any part of ospray that 'depends' on a given
# component can simply call 'OSPRAY_BUILD_COMPONENT(...)' without
# having to worry about whether this component is already built - if
# it isn't, then it is going to be built by calling this macro; while
# if it already is built, we don't have to worry about building it
# twice.

SET(OSPRAY_LIST_OF_ALREADY_BUILT_COMPONENTS "" CACHE INTERNAL "" FORCE)
SET(OSPRAY_COMPONENTS_ROOT "${PROJECT_SOURCE_DIR}/components" CACHE INTERNAL "" FORCE)

INCLUDE_DIRECTORIES(${OSPRAY_COMPONENTS_ROOT})

# the macro any part of ospray can use to request ospray to
# include/build a specific component
MACRO(OSPRAY_BUILD_COMPONENT comp)
  SET(CURRENT_COMPONENT_DIR ${COMPONENTS_DIR}/${comp})
  IF(NOT EXISTS ${CURRENT_COMPONENT_DIR})
    MESSAGE(FATAL_ERROR "Could not find component '${comp}'!")
  ENDIF()
    
  IF (";${OSPRAY_LIST_OF_ALREADY_BUILT_COMPONENTS};" MATCHES ";${comp};")
    # component already built; nothing to do!
  ELSE()
    SET(INCLUDED_AS_AN_OSPRAY_COMPONENT ON)
    SET(OSPRAY_LIST_OF_ALREADY_BUILT_COMPONENTS
        ${OSPRAY_LIST_OF_ALREADY_BUILT_COMPONENTS} ${comp}
        CACHE INTERNAL "" FORCE)

    # make sure that components explicitly set their install COMPONENT
    # otherwise they would inherit the default COMPONENT of the
    # requesting target, leading to inconsistent behaviour
    SET(DEFAULT_COMPONENT ${OSPRAY_DEFAULT_COMPONENT})
    UNSET(OSPRAY_DEFAULT_COMPONENT)

    ADD_SUBDIRECTORY(${CURRENT_COMPONENT_DIR}
      ${CMAKE_BINARY_DIR}/built_components/${comp})

    SET(OSPRAY_DEFAULT_COMPONENT ${DEFAULT_COMPONENT})

  ENDIF()
  
    IF(EXISTS ${CURRENT_COMPONENT_DIR}/include.cmake)
      INCLUDE(${CURRENT_COMPONENT_DIR}/include.cmake)
    ENDIF()

ENDMACRO()
